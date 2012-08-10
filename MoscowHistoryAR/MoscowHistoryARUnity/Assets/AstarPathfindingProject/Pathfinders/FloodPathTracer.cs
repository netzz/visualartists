using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Pathfinding
{
	
	/** Restrict suitable nodes by pathID.
	 * 
	  * Suitable nodes are in addition to the basic contraints, only the nodes which have a pathID equal to the specified path's pathID
	  * \see Path::pathID
	  * \see Node::pathID
	  * 
	  * \astarpro
	  */
	public class PathIDConstraint : NNConstraint {
		
		/** The path from which to grab the pathID to constrain on (Path::pathID) */
		private Path path;
		
		public void SetPath (Path path) {
			if (path == null) { Debug.LogWarning ("PathIDConstraint should not be used with a NULL path"); }
			this.path = path;
		}
		
		public override bool Suitable (Node node)
		{
			return node.GetNodeRun(path.runData).pathID == path.pathID && base.Suitable (node);
		}
	}
	
	/** Traces a path created with the Pathfinding::FloodPath.
	 * 
	 * See Pathfinding::FloodPath for examples on how to use this path type
	 * 
	 * \shadowimage{floodPathExample.png}
	 * \astarpro
	 * \ingroup paths */
	public class FloodPathTracer : Path
	{
		
		/** Reference to the FloodPath which searched the path originally */
		protected FloodPath flood;
		
		
		public FloodPathTracer (Vector3 start, FloodPath flood, OnPathDelegate callbackDelegate) : base (start,flood.originalStartPoint,callbackDelegate) {
			this.flood = flood;
			if (flood == null || !flood.processed)
				throw new System.ArgumentNullException ("You must supply a calculated FloodPath to the 'flood' argument");
			hasEndPoint = false;
			nnConstraint = new PathIDConstraint ();
		}
		
		public override void Prepare () {
			PathIDConstraint pic = nnConstraint as PathIDConstraint;
			if (pic == null) {
				Debug.LogWarning ("It is not recommended to call a FloodPathTracer path with a NNConstraint not inheriting from PathIDConstraint");
			} else {
				pic.SetPath (flood);
			}
			
			base.Prepare ();
			
		}
		
		/** Initializes the path. Sets up the open list and adds the first node to it */
		public override void Initialize () {
			
			System.DateTime startTime = System.DateTime.Now;
			if (startNode != null) {
				Trace (startNode.GetNodeRun(flood.runData));
				foundEnd = true;
			} else {
				LogError ("Could not find valid start node");
			}
			
			duration += (System.DateTime.Now.Ticks-startTime.Ticks)*0.0001F;
			
		}
		
		public override void CalculateStep (long targetTick) {
			if (!IsDone ()) {
				LogError ("Something went wrong. At this point the path should be completed");
			}
		}
		
		/** Traces the calculated path from the end node to the start.
		 * This will build an array (#path) of the nodes this path will pass through and also set the #vectorPath array to the #path arrays positions.
		 * This function will not revert the path as the original implementation does, so the path will go from \a from to the parent root node (usually the start node) */
		public override void Trace (NodeRun from) {
			
			int count = 0;
			
			NodeRun c = from;
			while (c != null) {
				c = c.parent;
				count++;
				if (count > 1024) {
					Debug.LogWarning ("Inifinity loop? >1024 node path");
					break;
				}
			}
			
			path = new Node[count];
			c = from;
			
			for (int i = 0;i<count;i++) {
				path[i] = c.node;
				c = c.parent;
			}
			
			vectorPath = new Vector3[count];
			
			for (int i=0;i<count;i++) {
				vectorPath[i] = (Vector3)path[i].position;
			}
		}
	}
}

