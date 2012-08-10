//#define DEBUGGING
//#define SafeIntMath
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using JsonFx.Json;

namespace Pathfinding {
	[System.Serializable]
	[JsonOptIn]
	/** Automatically generates navmesh graphs based on world geometry.
The recast graph is based on Recast (http://code.google.com/p/recastnavigation/).\n
I have translated a good portion of it to C# to run it natively in Unity. The Recast process is described as follows:
 - The voxel mold is build from the input triangle mesh by rasterizing the triangles into a multi-layer heightfield.
 Some simple filters are then applied to the mold to prune out locations where the character would not be able to move.
 - The walkable areas described by the mold are divided into simple overlayed 2D regions.
 The resulting regions have only one non-overlapping contour, which simplifies the final step of the process tremendously.
 - The navigation polygons are peeled off from the regions by first tracing the boundaries and then simplifying them.
 The resulting polygons are finally converted to convex polygons which makes them perfect for pathfinding and spatial reasoning about the level.
 
It works exactly like that in the C# version as well, except that everything is triangulated to triangles instead of n-gons.
The recast generation process completely ignores colliders since it only works on an existing "polygon soup".
This is usually a good thing though, because world geometry is usually more detailed than the colliders.

\section export Exporting for manual editing
In the editor there is a button for exporting the generated graph to a .obj file.
Usually the generation process is good enough for the game directly, but in some cases you might want to edit some minor details.
So you can export the graph to a .obj file, open it in your favourite 3D application, edit it, and export it to a mesh which Unity can import.
Then you assign that new mesh to the "Replacement Mesh" field right below the export button. That mesh will then be used to calculate the graph.

Since many 3D modelling programs use different axis systems (unity uses X=right, Y=up, Z=forward), it can be a bit tricky to get the rotation and scaling right.
For blender for example, what you have to do is to first import the mesh using the .obj importer. Don't change anything related to axes in the settings.
Then select the mesh, open the transform tab (usually the thin toolbar to the right of the 3D view) and set Scale -> Z to -1.
If you transform it using the S (scale) hotkey, it seems to set both Z and Y to -1 of some reason.
Then make the edits you need and export it as an .obj file to somewhere in the Unity project.
But this time, edit the setting named "Forward" to "Z forward" (not -Z as it is per default).

\shadowimage{recastgraph_graph.png}
\shadowimage{recastgraph_inspector.png}


	  * \ingroup graphs
	 * \astarpro */
	public class RecastGraph : NavGraph, INavmesh, ISerializableGraph, IRaycastableGraph, IFunnelGraph, IUpdatableGraph {
		
		public override Node[] CreateNodes (int number) {
			MeshNode[] tmp = new MeshNode[number];
			for (int i=0;i<number;i++) {
				tmp[i] = new MeshNode ();
			}
			return tmp as Node[];
		}
		
		//public int erosionRadius = 2; /*< Voxels to erode away from the edges of the mesh */
		
		[JsonMember]
		public float characterRadius = 0.5F;
		
		[JsonMember]
		public float contourMaxError = 2F; /**< Max distance from simplified edge to real edge */
		
		[JsonMember]
		public float cellSize = 0.5F; /**< Voxel sample size (x,z) */
		
		[JsonMember]
		public float cellHeight = 0.4F; /**< Voxel sample size (y) */
		
		[JsonMember]
		public float walkableHeight = 2F; /**< Character height*/
		
		[JsonMember]
		public float walkableClimb = 0.5F; /**< Height the character can climb */
		
		[JsonMember]
		public float maxSlope = 30; /**< Max slope in degrees the character can traverse */
		
		[JsonMember]
		public float maxEdgeLength = 20; /**< Longer edges will be subdivided. Reducing this value can improve path quality since similarly sized polygons yield better paths than really large and really small next to each other */
		
		[JsonMember]
		public int regionMinSize = 8;
		
		/** Use a C++ version of Recast.
		 * The C++ version is faster and has more features, though right now the features are quite similar because I haven't added support for them yet.\n
		 * The C++ version can only be used in the editor or in standalones and it requires a special file to be placed inside the executable for standalones.\n
		 * When deploying for mac, the Recast file (which can be found in the AstarPathfindingProject folder) should be copied to a new folder myApplication.app/Contents/Recast/ 
		 * \shadowimage{/images/recastMacPlacement.png}
		 * When deploying for windows, the Recast file should be copied to the data folder/Recast/ 
		 * \shadowimage{images/recastWindowsPlacement.png}
		 * You can however save a cache of the graph scanned in the editor and use that in a webplayer but you cannot rescann any graphs in the webplayer\n
		 * I have no information on whether or not it would work using iPhone or Android, so I would refrain form trying to use it there (though you can still cache graphs as you can with the webplayer) 
		 */
		[JsonMember]
		public bool useCRecast = true;
		
		[JsonMember]
		public bool includeOutOfBounds = false;
		
		/** Center of the bounding box. Scanning will only be done inside the bounding box */
		[JsonMember]
		public Vector3 forcedBoundsCenter;
		
		/** Size of the bounding box. */
		[JsonMember]
		public Vector3 forcedBoundsSize = new Vector3 (100,40,100);
		
		/** Masks which objects to include */
		[JsonMember]
		public LayerMask mask = -1;
		
		/** Show an outline of the polygons in the Unity Editor */
		[JsonMember]
		public bool showMeshOutline = false;
		
		/** Include the Terrain in the scene. */
		[JsonMember]
		public bool includeTerrain = true;
		
		/** Controls how large the sample size for the terrain is. A higher value is faster but less accurate */
		[JsonMember]
		public int terrainSampleSize = 3;
		
		/** More accurate nearest node queries.
		 * When on, looks for the closest point on every triangle instead of if point is inside the node triangle in XZ space.
		 * This is slower, but a lot better if your mesh contains overlaps (e.g bridges over other areas of the mesh).
		 * Note that for maximum effect the Full Get Nearest Node Search setting should be toggled in A* Inspector Settings.
		 */
		[JsonMember]
		public bool accurateNearestNode = true;
		
		public Bounds forcedBounds {
			get {
				return new Bounds (forcedBoundsCenter,forcedBoundsSize);
			}
		}
		
		/** Bounding Box Tree. Enables really fast lookups of nodes.
		 * \astarpro */
		BBTree _bbTree;
		public BBTree bbTree {
			get { return _bbTree; }
			set { _bbTree = value;}
		}
		
		Int3[] _vertices;
		
		public Int3[] vertices {
			get {
				return _vertices;
			}
			set {
				_vertices = value;
			}
		}
		
		Vector3[] _vectorVertices;
		
		public Vector3[] vectorVertices {
			get {
				if (_vectorVertices != null && _vectorVertices.Length == vertices.Length) {
					return _vectorVertices;
				}
				if (vertices == null) return null;
				_vectorVertices = new Vector3[vertices.Length];
				for (int i=0;i<_vectorVertices.Length;i++) {
					_vectorVertices[i] = (Vector3)vertices[i];
				}
				return _vectorVertices;
			}
		}
		
		public void SnapForceBoundsToScene () {
			
			List<MeshFilter> filteredFilters = GetSceneMeshes ();
			
			if (filteredFilters.Count == 0) {
				return;
			}
			
			Bounds bounds = new Bounds ();
			
			for (int i=0;i<filteredFilters.Count;i++) {
				if (filteredFilters[i].renderer != null) {
					bounds = filteredFilters[i].renderer.bounds;
					break;
				}
			}
			
			for (int i=0;i<filteredFilters.Count;i++) {
				if (filteredFilters[i].renderer != null) {
					bounds.Encapsulate (filteredFilters[i].renderer.bounds);
				}
			}
			
			forcedBoundsCenter = bounds.center;
			forcedBoundsSize = bounds.size;
		}
		
		public List<MeshFilter> GetSceneMeshes () {
			MeshFilter[] filters = GameObject.FindObjectsOfType(typeof(MeshFilter)) as MeshFilter[];
			
			List<MeshFilter> filteredFilters = new List<MeshFilter> (filters.Length/3);
			foreach (MeshFilter filter in filters) {
				if (((1 << filter.gameObject.layer) & mask) == (1 << filter.gameObject.layer)) {
					filteredFilters.Add (filter);
				}
			}
			
			return filteredFilters;
		}
		
		public void UpdateArea (GraphUpdateObject guo) {
			NavMeshGraph.UpdateArea (guo, this);
		}
		
		public override NNInfo GetNearest (Vector3 position, NNConstraint constraint, Node hint) {
			return NavMeshGraph.GetNearest (this, nodes,position, constraint, accurateNearestNode);
		}
		
		public override NNInfo GetNearestForce (Vector3 position, NNConstraint constraint) {
			return NavMeshGraph.GetNearestForce (nodes,vertices,position,constraint, accurateNearestNode);
		}
		
		public void BuildFunnelCorridor (Node[] path, int startIndex, int endIndex, List<Vector3> left, List<Vector3> right) {
			NavMeshGraph.BuildFunnelCorridor (this,path,startIndex,endIndex,left,right);
		}
		
		public void AddPortal (Node n1, Node n2, List<Vector3> left, List<Vector3> right) {
		}
		
		public struct ExtraMesh {
			public Vector3[] vertices;
			public int[] triangles;
			public Bounds bounds;
			
			public ExtraMesh (Vector3[] v, int[] t, Bounds b) {
				vertices = v;
				triangles = t;
				bounds = b;
			}
		}
		
#if UNITY_EDITOR
		public static bool IsJsEnabled () {
			if (System.IO.Directory.Exists (Application.dataPath+"/AstarPathfindingEditor/Editor")) {
				return true;
			}
			return false;
		}
#endif
		
		public static string GetRecastPath () {
#if UNITY_EDITOR
			if (IsJsEnabled ()) {
				return Application.dataPath + "/Plugins/AstarPathfindingProject/recast";
			} else {
				return Application.dataPath + "/AstarPathfindingProject/recast";
			}
#else
			return Application.dataPath + "/Recast/recast";
#endif
		}
		
#if !PhotonImplementation
		
		public override void Scan () {
			AstarProfiler.Reset ();
			//AstarProfiler.StartProfile ("Base Scan");
			
			//base.Scan ();
			
			//AstarProfiler.EndProfile ("Base Scan");
			if (useCRecast) {
				ScanCRecast ();
				
			} else {
			
				MeshFilter[] filters;
				ExtraMesh[] extraMeshes;
				
				if (!CollectMeshes (out filters, out extraMeshes)) {
					nodes = new Node[0];
					return;
				}
				
				Voxelize vox = new Voxelize (cellHeight, cellSize, walkableClimb, walkableHeight, maxSlope);
				
				vox.maxEdgeLength = maxEdgeLength;
				vox.forcedBounds = forcedBounds;
				vox.includeOutOfBounds = includeOutOfBounds;
				
				
				//g.GetComponent<Voxelize>();
				vox.VoxelizeMesh (filters, extraMeshes);
				
				/*bool[,] open = new bool[width,depth];
				int[,] visited = new int[width+1,depth+1];
				
				for (int z=0;z<depth;z++) {
					for (int x = 0;x < width;x++) {
						open[x,z] = graphNodes[z*width+x].walkable;
					}
				}*/
				
				/*for (int i=0;i<depth*width;i++) {
					open[i] = graphNodes[i].walkable;
				}
				
				
				int wd = width*depth;
				
				List<int> boundary = new List<int>();
				
				int p = 0;
				
				for (int i=0;i<wd;i++) {
					if (!open[i]) {
						boundary.Add (i);
						
						p = i;
						
						int backtrack = i-1;
						
						
					}*/
					
				vox.ErodeWalkableArea (Mathf.CeilToInt (2*characterRadius/cellSize));
				
				
				vox.BuildDistanceField ();
				
				vox.BuildRegions ();
				
				VoxelContourSet cset = new VoxelContourSet ();
				
				vox.BuildContours (contourMaxError,1,cset,Voxelize.RC_CONTOUR_TESS_WALL_EDGES);
				
				VoxelMesh mesh;
				
				vox.BuildPolyMesh (cset,3,out mesh);
				
				
				Vector3[] vertices = new Vector3[mesh.verts.Length];
				
				AstarProfiler.StartProfile ("Build Nodes");
				
				for (int i=0;i<vertices.Length;i++) {
					vertices[i] = (Vector3)mesh.verts[i];
				}
				
				matrix = Matrix4x4.TRS (vox.voxelOffset,Quaternion.identity,Int3.Precision*Voxelize.CellScale);
				//Int3.Precision*Voxelize.CellScale+(Int3)vox.voxelOffset
				
				//GenerateNodes (this,vectorVertices,triangles, out originalVertices, out _vertices);
				
				NavMeshGraph.GenerateNodes (this,vertices,mesh.tris, out _vectorVertices, out _vertices);
				
				AstarProfiler.EndProfile ("Build Nodes");
				
				AstarProfiler.PrintResults ();
				
			}
		}
		
		public void ScanCRecast () {
	//#if (!UNITY_EDITOR && !UNITY_STANDALONE_OSX && !UNITY_STANDALONE_WIN) || UNITY_WEBPLAYER || UNITY_IPHONE || UNITY_ANDROID || UNITY_DASHBOARD_WIDGET || UNITY_XBOX360 || UNITY_PS3
	#if (UNITY_STANDALONE_OSX || UNITY_STANDALONE_WIN)
			System.Diagnostics.Process myProcess = new System.Diagnostics.Process();
			myProcess.StartInfo.FileName = GetRecastPath ();//"/Users/arong/Recast/build/Debug/Recast";
			//System.Diagnostics.ProcessStartInfo startInfo = new System.Diagnostics.ProcessStartInfo ();
			//startInfo.UseShellExecute = true;
			myProcess.StartInfo.UseShellExecute = false;
			myProcess.StartInfo.RedirectStandardInput = true;
			myProcess.StartInfo.RedirectStandardOutput = true;
			myProcess.StartInfo.Arguments = "";
			
			MeshFilter[] filters;
			ExtraMesh[] extraMeshes;
			//Get all meshes which should be used
			CollectMeshes (out filters, out extraMeshes);
		
			Vector3[] inputVerts;
			int[] inputTris;
			//Get polygon soup from meshes
			Voxelize.CollectMeshes (filters,extraMeshes,forcedBounds,out inputVerts, out inputTris);
			
			//Bild .obj file
			System.Text.StringBuilder arguments = new System.Text.StringBuilder ();
			arguments.Append ("o recastMesh.obj\n");
			for (int i=0;i<inputVerts.Length;i++) {
				arguments.Append ("v "+inputVerts[i].x.ToString ("0.000")+" ").Append (inputVerts[i].y.ToString ("0.000")+" ").Append (inputVerts[i].z.ToString ("0.000"));
				arguments.Append ("\n");
			}
			
			//Build .obj file tris
			for (int i=0;i<inputTris.Length-2;i+=3) {
				arguments.Append ("f "+(inputTris[i]+1)+"//0 ").Append ((inputTris[i+1]+1)+"//0 ").Append ((inputTris[i+2]+1)+"//0");
				//Debug.DrawLine (inputVerts[inputTris[i]],inputVerts[inputTris[i+1]],Color.red);
				//Debug.DrawLine (inputVerts[inputTris[i+1]],inputVerts[inputTris[i+2]],Color.red);
				//Debug.DrawLine (inputVerts[inputTris[i+2]],inputVerts[inputTris[i]],Color.red);
				
				arguments.Append ("\n");
			}
			
			string tmpPath = System.IO.Path.GetTempPath ();
			tmpPath += "recastMesh.obj";
			
			try {
				using (System.IO.StreamWriter outfile = new System.IO.StreamWriter(tmpPath)) {
					outfile.Write (arguments.ToString ());
				}
				
				myProcess.StartInfo.Arguments = tmpPath
					+"\n"+cellSize+"\n"+
						cellHeight+"\n"+
						walkableHeight+"\n"+
						walkableClimb+"\n"+
						maxSlope+"\n"+
						maxEdgeLength+"\n"+
						contourMaxError+"\n"+
						regionMinSize+"\n"+
						characterRadius;
				
				
		/*		public int erosionRadius = 2; /< Voxels to erode away from the edges of the mesh /
		public float contourMaxError = 2F; /< Max distance from simplified edge to real edge /
		
		public float cellSize = 0.5F; /< Voxel sample size (x,z) /
		public float cellHeight = 0.4F; /< Voxel sample size (y) /
		public float walkableHeight = 2F; /< Character height/
		public float walkableClimb = 0.5F; /< Height the character can climb /
		public float maxSlope = 30; /< Max slope in degrees the character can traverse /
		public float maxEdgeLength = 20; /< Longer edges will be subdivided. Reducing this value can im
		public bool useCRecast = true;
		public bool includeOutOfBounds = false;*/
				
				myProcess.Start ();
				System.IO.StreamReader sOut = myProcess.StandardOutput;
				
				//string result = sOut.ReadToEnd ();
				//Debug.Log (result);
				//return;
					
				bool failed = false;
				bool startedVerts = false;
				int readVerts = 0;
				bool startedTris = false;
				int vCount = -1;
				int readTris = 0;
				int trisCount = 0;
				Vector3[] verts = null;
				int[] tris = null;
				int internalVertCount = 0;
				
				Vector3 bmin = Vector3.zero;
				
				float cs = 1F;
				float ch = 1F;
				
				while (sOut.Peek() >= 0) 
		        {
					string line = sOut.ReadLine();
					int resultInt;
					
					if (line == "") {
						continue;
					}
					
		            if (!int.TryParse (line, out resultInt)) {
						//Debug.Log ("Syntax Error at '"+line+"'");
						failed = true;
						break;
					}
					
					if (!startedVerts) {
						verts = new Vector3[resultInt];
						
						if (resultInt == 0) {
							failed = true;
							break;
						}
						
						bmin.x = float.Parse (sOut.ReadLine());
						bmin.y = float.Parse (sOut.ReadLine());
						bmin.z = float.Parse (sOut.ReadLine());
						cs = float.Parse (sOut.ReadLine());
						ch = float.Parse (sOut.ReadLine());
						
						startedVerts = true;
						//Debug.Log ("Starting Reading "+resultInt+" verts "+bmin.ToString ()+" - "+cs+" * "+ch);
					} else if (readVerts < verts.Length) {
						resultInt *= 1;
						if (internalVertCount == 0) {
							verts[readVerts].x = resultInt*cs + bmin.x;
						} else if (internalVertCount == 1) {
							verts[readVerts].y = resultInt*ch + bmin.y;
						} else {
							verts[readVerts].z = resultInt*cs + bmin.z;
						}
						
						internalVertCount++;
						
						if (internalVertCount == 3) {
							internalVertCount = 0;
							readVerts++;
						}
						
					} else if (!startedTris) {
						
						trisCount = resultInt;
						startedTris = true;
					} else if (vCount == -1) {
						vCount = resultInt;
						tris = new int[trisCount*vCount];
						//Debug.Log ("Starting Reading "+trisCount+" - "+tris.Length+" tris at vertsCount "+readVerts);
						//Debug.Log ("Max vertices per polygon: "+vCount);
						
					} else if (readTris < tris.Length) {
						tris[readTris] = resultInt;
						readTris++;
					}
		        }
				
				if (!myProcess.HasExited)
		        {
		            myProcess.Kill();
		        }
				
				sOut.Close();
		    	myProcess.Close();
				
				if (failed) {
					return;
				}
					
				matrix = Matrix4x4.TRS (Vector3.zero,Quaternion.identity,Vector3.one);
				
				NavMeshGraph.GenerateNodes (this,verts,tris, out _vectorVertices, out _vertices);
			} finally {
				//Debug.Log (tmpPath);
				System.IO.File.Delete (tmpPath);
			}
	#else
			Debug.LogError ("The C++ version of recast can only be used in editor or standalone mode, I'm sure it cannot be used in the webplayer, but other platforms are not tested yet\n" +
				"If you are in the Unity Editor, try switching Platform to Standalone just when scanning, scanned graphs can be cached to enable them to be used in a webplayer");
			_vectorVertices = new Vector3[0];
			_vertices = new Int3[0];
			nodes = new Node[0];
	#endif
		}
		
		public bool CollectMeshes (out MeshFilter[] filters, out ExtraMesh[] extraMeshes) {
			List<MeshFilter> filteredFilters = GetSceneMeshes ();
			
			List<ExtraMesh> extraMeshesList = new List<ExtraMesh> ();
			
			if (includeTerrain && Terrain.activeTerrain != null && Terrain.activeTerrain.terrainData != null) {
				
				TerrainData terrainData = Terrain.activeTerrain.terrainData;
				float[,] heights = terrainData.GetHeights (0,0,terrainData.heightmapWidth,terrainData.heightmapHeight);
				
				terrainSampleSize = terrainSampleSize < 1 ? 1 : terrainSampleSize;//Clamp to at least 1
				
				int hWidth = terrainData.heightmapWidth / terrainSampleSize;
				int hHeight = terrainData.heightmapHeight / terrainSampleSize;
				Vector3[] terrainVertices = new Vector3[hWidth*hHeight];
				
				Vector3 hSampleSize = terrainData.heightmapScale;
				Vector3 offset = Terrain.activeTerrain.GetPosition ();
				float heightScale = terrainData.size.y;
				
				for (int z = 0, nz = 0;nz < hHeight;z+= terrainSampleSize, nz++) {
					for (int x = 0, nx = 0; nx < hWidth;x+= terrainSampleSize, nx++) {
						terrainVertices[nz*hWidth + nx] = new Vector3 (z * hSampleSize.z,heights[x,z]*heightScale, x * hSampleSize.x) + offset;
					}
				}
				
				int[] tris = new int[(hWidth-1)*(hHeight-1)*2*3];
				int c = 0;
				for (int z = 0;z < hHeight-1;z++) {
					for (int x = 0; x < hWidth-1;x++) {
						tris[c] = z*hWidth + x;
						tris[c+1] = z*hWidth + x+1;
						tris[c+2] = (z+1)*hWidth + x+1;
						c += 3;
						tris[c] = z*hWidth + x;
						tris[c+1] = (z+1)*hWidth + x+1;
						tris[c+2] = (z+1)*hWidth + x;
						c += 3;
					}
				}
				
				for (int i=0;i<tris.Length;i+=3) {
					Debug.DrawLine (terrainVertices[tris[i]],terrainVertices[tris[i+1]],Color.red);
					Debug.DrawLine (terrainVertices[tris[i+1]],terrainVertices[tris[i+2]],Color.red);
					Debug.DrawLine (terrainVertices[tris[i+2]],terrainVertices[tris[i]],Color.red);
				}
				
				Vector3 center = offset + terrainData.size * 0.5F;
				Bounds b = new Bounds (center, terrainData.size);
				
				extraMeshesList.Add (new ExtraMesh (terrainVertices,tris,b));
				/*terrainMesh.vertices = terrainVertices;
				terrainMesh.triangles = tris;
				terrainMesh.RecalculateBounds ();
				terrainFilter.mesh = terrainMesh;
				filteredFilters.Add (terrainFilter);
				}*/
			}
			
			if (filteredFilters.Count == 0 && extraMeshesList.Count == 0) {
				Debug.LogWarning ("No MeshFilters where found contained in the layers specified by the 'mask' variable");
				filters = null;
				extraMeshes = null;
				return false;
			}
			
			filters = filteredFilters.ToArray ();
			extraMeshes = extraMeshesList.ToArray ();
			return true;
		}
#endif
		
		
		public bool Linecast (Vector3 origin, Vector3 end) {
			return Linecast (origin, end, GetNearest (origin, NNConstraint.None));
		}
		
		public bool Linecast (Vector3 origin, Vector3 end, Node hint, out GraphHitInfo hit) {
			return NavMeshGraph.Linecast (this as INavmesh, origin,end,hint,false,0, out hit);
		}
		
		public bool Linecast (Vector3 origin, Vector3 end, Node hint) {
			GraphHitInfo hit;
			return NavMeshGraph.Linecast (this as INavmesh, origin,end,hint,false,0, out hit);
		}
		
		public void Sort (Int3[] a) {
			
			bool changed = true;
		
			while (changed) {
				changed = false;
				for (int i=0;i<a.Length-1;i++) {
					if (a[i].x > a[i+1].x || (a[i].x == a[i+1].x && (a[i].y > a[i+1].y || (a[i].y == a[i+1].y && a[i].z > a[i+1].z)))) {
						Int3 tmp = a[i];
						a[i] = a[i+1];
						a[i+1] = tmp;
						changed = true;
					}
				}
			}
		}
		
#if !PhotonImplementation
		public override void OnDrawGizmos (bool drawNodes) {
			
			if (!drawNodes) {
				return;
			}
			
			if (bbTree != null) {
				bbTree.OnDrawGizmos ();
			}
			
			Gizmos.DrawWireCube (forcedBounds.center,forcedBounds.size);
			//base.OnDrawGizmos (drawNodes);
			
			if (nodes == null) {
				//Scan (AstarPath.active.GetGraphIndex (this));
			}
			
			if (nodes == null) {
				return;
			}
			
			for (int i=0;i<nodes.Length;i++) {
				
				//AstarColor.NodeConnection;
				
				MeshNode node = (MeshNode)nodes[i];
				
				
				if (AstarPath.active.debugPathData != null && AstarPath.active.showSearchTree && node.GetNodeRun(AstarPath.active.debugPathData).parent != null) {
					//Gizmos.color = new Color (0,1,0,0.7F);
					Gizmos.color = NodeColor (node,AstarPath.active.debugPathData);
					Gizmos.DrawLine ((Vector3)node.position,(Vector3)node.GetNodeRun(AstarPath.active.debugPathData).parent.node.position);
				} else {
					//for (int q=0;q<node.connections.Length;q++) {
					//	Gizmos.DrawLine (node.position,node.connections[q].position);
					//}
				}
				
				
				/*Gizmos.color = AstarColor.MeshEdgeColor;
				for (int q=0;q<node.connections.Length;q++) {
					//Gizmos.color = Color.Lerp (Color.green,Color.red,node.connectionCosts[q]/8000F);
					Gizmos.DrawLine (node.position,node.connections[q].position);
				}*/
				
				//Gizmos.color = NodeColor (node);
				//Gizmos.color.a = 0.2F;
				
				if (showMeshOutline) {
					Gizmos.color = NodeColor (node,AstarPath.active.debugPathData);
					Gizmos.DrawLine ((Vector3)vertices[node.v1],(Vector3)vertices[node.v2]);
					Gizmos.DrawLine ((Vector3)vertices[node.v2],(Vector3)vertices[node.v3]);
					Gizmos.DrawLine ((Vector3)vertices[node.v3],(Vector3)vertices[node.v1]);
				}
			}
			
		}
#endif
		
		public override byte[] SerializeExtraInfo () {
			return NavMeshGraph.SerializeMeshNodes (this,nodes);
		}
		
		public override void DeserializeExtraInfo (byte[] bytes) {
			NavMeshGraph.DeserializeMeshNodes (this,nodes,bytes);
		}
		
#region OldSerializer
		
		public void SerializeNodes (Node[] nodes, AstarSerializer serializer) {
			NavMeshGraph.SerializeMeshNodes (this as INavmesh, nodes, serializer);
		}
		
		public void DeSerializeNodes (Node[] nodes, AstarSerializer serializer) {
			NavMeshGraph.DeSerializeMeshNodes (this as INavmesh, nodes, serializer);
		}
		
		public void SerializeSettings (AstarSerializer serializer) {
			//serializer.AddValue ("erosionRadius",erosionRadius);
			serializer.AddValue ("contourMaxError",contourMaxError);
			
			serializer.AddValue ("cellSize",cellSize);
			serializer.AddValue ("cellHeight",cellHeight);
			serializer.AddValue ("walkableHeight",walkableHeight);
			serializer.AddValue ("walkableClimb",walkableClimb);
			serializer.AddValue ("maxSlope",maxSlope);
			serializer.AddValue ("maxEdgeLength",maxEdgeLength);
			
			serializer.AddValue ("forcedBoundsCenter",forcedBoundsCenter);
			serializer.AddValue ("forcedBoundsSize",forcedBoundsSize);
			
			serializer.AddValue ("mask",mask.value);
			
			serializer.AddValue ("showMeshOutline",showMeshOutline);
			
			serializer.AddValue ("includeOutOfBounds",includeOutOfBounds);
			
			serializer.AddValue ("regionMinSize",regionMinSize);
			serializer.AddValue ("characterRadius",characterRadius);
			serializer.AddValue ("useCRecast",useCRecast);
			
		}
		
		public void DeSerializeSettings (AstarSerializer serializer) {
			//erosionRadius = (int)serializer.GetValue ("erosionRadius",typeof(int));
			contourMaxError = (float)serializer.GetValue ("contourMaxError",typeof(float));
			
			cellSize = (float)serializer.GetValue ("cellSize",typeof(float));
			cellHeight = (float)serializer.GetValue ("cellHeight",typeof(float));
			walkableHeight = (float)serializer.GetValue ("walkableHeight",typeof(float));
			walkableClimb = (float)serializer.GetValue ("walkableClimb",typeof(float));
			maxSlope = (float)serializer.GetValue ("maxSlope",typeof(float));
			maxEdgeLength = (float)serializer.GetValue ("maxEdgeLength",typeof(float));
			
			forcedBoundsCenter = (Vector3)serializer.GetValue ("forcedBoundsCenter",typeof(Vector3));
			forcedBoundsSize = (Vector3)serializer.GetValue ("forcedBoundsSize",typeof(Vector3));
			
			mask.value = (int)serializer.GetValue ("mask",typeof(int));
			
			showMeshOutline = (bool)serializer.GetValue ("showMeshOutline",typeof(bool));
			
			includeOutOfBounds = (bool)serializer.GetValue ("includeOutOfBounds",typeof(bool));
			
			regionMinSize = (int)serializer.GetValue ("regionMinSize",typeof(int));
			characterRadius = (float)serializer.GetValue ("characterRadius",typeof(float));
			useCRecast = (bool)serializer.GetValue ("useCRecast",typeof(bool));
		}
		
#endregion
		
		public struct Int2 {
			public int x;
			public int z;
			
			public Int2 (int _x, int _z) {
				x = _x;
				z = _z;
			}
		}
	}
	
	/*------- From Wikipedia -------
	Input: A square tessellation, T, containing a connected component P of black cells. Output: A sequence B (b1, b2 ,..., bk) of boundary pixels i.e. the contour. Define M(a) to be the Moore neighborhood of pixel a. Let p denote the current boundary pixel. Let c denote the current pixel under consideration i.e. c is in M(p).
	*/
	
	/*Begin
	  Set B to be empty.
	  From bottom to top and left to right scan the cells of T until a black pixel, s, of P is found.
	  Insert s in B.
	  Set the current boundary point p to s i.e. p=s
	  Backtrack i.e. move to the pixel from which s was entered.
	  Set c to be the next clockwise pixel in M(p).
	  While c not equal to s do
	  If c is black
	  insert c in B
	  set p=c
	  backtrack (move the current pixel c to the pixel from which p was entered)
	  else
	  advance the current pixel c to the next clockwise pixel in M(p)
	  end While
	  End*/
}
