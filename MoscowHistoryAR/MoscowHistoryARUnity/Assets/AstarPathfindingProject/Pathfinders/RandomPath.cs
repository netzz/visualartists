using UnityEngine;
using Pathfinding;

namespace Pathfinding {
	/** Finds a path in a random direction from the start node.
	 * \ingroup paths
	 * Terminates and returns when G \>= \a length (passed to the constructor) + RandomPath::spread or when there are no more nodes left to search.\n
	 * 
	 * \code

//Call a RandomPath call like this, assumes that a Seeker is attached to the GameObject

//The path will be returned when the path is over a specified length (or more accurately has "costed" more than a specific value)
int theGScoreToStopAt = 50;

//Create a path object
RandomPath path = new RandomPath  (transform.position, theGScoreToStopAt);

//Get the Seeker component which must be attached to this GameObject
Seeker seeker = GetComponent<Seeker>();

//Start the path and return the result to MyCompleteFunction (which is a function you have to define, the name can of course be changed)
seeker.StartPath (path,MyCompleteFunction);

	 * \endcode
	 * \astarpro */
	public class RandomPath : Path {
		
		/** G score to stop searching at.
		  * The G score is rougly the distance to get from the start node to a node multiplied by 100 (per default, see Pathfinding::Int3::Precision), plus any eventual penalties */
		public int searchLength = 0;
		
		/** All G scores between #searchLength and #searchLength+#spread are valid end points, a random one of them is chosen as the final point.
		 * On grid graphs a low spread usually works (but keep it higher than nodeSize*100 since that it the default cost of moving between two nodes), on NavMesh graphs
		 * I would recommend a higher spread so it can evaluate more nodes */
		public int spread = 100;
		
		/** Chance that the currently chosen node for end node is replaced by a later found node (evaluated every time a new node is found).
		 * The default value of 0.1 usually works good enough. */
		public float replaceChance = 0.1F;
		
		/** If an #aim is set, the higher this value is, the more it will try to reach #aim */
		public float aimStrength = 0.0F;
		
		/** Currently chosen end node */
		NodeRun chosenNodeR = null;
		
		/** The node with the highest G score which is still lower than #searchLength.
		  * Used as a backup if a node with a G score higher than #searchLength could be found */
		NodeRun maxGScoreNodeR = null;
		
		/** The G score of #maxGScoreNode */
		int maxGScore = 0;
		
		/** An aim can be used to guide the pathfinder to not take totally random paths.
		 * For example you might want your AI to continue in generally the same direction as before, then you can specify
		 * aim to be transform.postion + transform.forward*10 which will make it more often take paths nearer that point
		 * \see #aimStrength */
		public Vector3 aim = Vector3.zero;
		
		/** Random class */
		System.Random rnd;
		
		public RandomPath (Vector3 start, int length, OnPathDelegate callbackDelegate = null) { 
			callTime = System.DateTime.Now;
		
			callback = callbackDelegate;
			
			searchLength = length;
			
			if (AstarPath.active == null || AstarPath.active.graphs == null) {
				errorLog += "No NavGraphs have been calculated yet - Don't run any pathfinding calls in Awake";
				if (AstarPath.active.logPathResults != PathLog.None) {
					Debug.LogError (errorLog);
				}
				error = true;
				return;
			}
			
			pathID = AstarPath.active.GetNextPathID ();
			
			originalStartPoint = start;
			originalEndPoint = Vector3.zero;
			
			startPoint = start;
			endPoint = Vector3.zero;
			
			startIntPoint = (Int3)start;
			hTarget = (Int3)aim;//(Int3)(start-aim);//new Int3(0,0,0);
			rnd = new System.Random ();
		}
		
		/** Not really necessary since this is a special path and will not be pooled, but why not */
		public override void Reset (Vector3 start, Vector3 end, OnPathDelegate callbackDelegate, bool reset = true) {
			base.Reset (start,end,callbackDelegate,reset);
			searchLength = 0;
		}
		
		/** Calls callback to return the calculated path.
		 * \see #callback */
		public override void ReturnPath () {
			if (path != null && path.Length > 0) {
				endNode = path[path.Length-1];
				endPoint = (Vector3)endNode.position;
				originalEndPoint = endPoint;
				
				hTarget = endNode.position;
			}
			if (callback != null) {
				callback (this);
			}
		}
		
		public override void Prepare () {
			System.DateTime startTime = System.DateTime.Now;
			
			maxFrameTime = AstarPath.active.maxFrameTime;
			
			nnConstraint.tags = enabledTags;
			NNInfo startNNInfo 	= AstarPath.active.GetNearest (startPoint,nnConstraint, startHint);
			
			startPoint = startNNInfo.clampedPosition;
			endPoint = startPoint;
			
			startIntPoint = (Int3)startPoint;
			hTarget = (Int3)aim;//startIntPoint;
			
			startNode = startNNInfo.node;
			endNode = startNode;
			
	#if DEBUG
			Debug.DrawLine (startNode.position,startPoint,Color.blue);
			Debug.DrawLine (endNode.position,endPoint,Color.blue);
	#endif
			
			if (startNode == null || endNode == null) {
				LogError ("Couldn't find close nodes to either the start or the end (start = "+(startNode != null)+" end = "+(endNode != null)+")");
				duration += (System.DateTime.Now.Ticks-startTime.Ticks)*0.0001F;
				return;
			}
			
			if (!startNode.walkable) {
				LogError ("The node closest to the start point is not walkable");
				duration += (System.DateTime.Now.Ticks-startTime.Ticks)*0.0001F;
				return;
			}
			
			//heuristic = Heuristic.None;
			heuristicScale = aimStrength;//0F;
			
			duration += (System.DateTime.Now.Ticks-startTime.Ticks)*0.0001F;
		}
		
		public override void Initialize () {
			runData.pathID = pathID;
			
			//Resets the binary heap, don't clear everything because that takes an awful lot of time, instead we can just change the numberOfItems in it (which is just an int)
			//Binary heaps are just like a standard array but are always sorted so the node with the lowest F value can be retrieved faster
			runData.open.Clear ();
			
			//Adjust the costs for the end node
			/*if (hasEndPoint && recalcStartEndCosts) {
				endNodeCosts = endNode.InitialOpen (open,hTarget,(Int3)endPoint,this,false);
				callback += ResetCosts; /* \todo Might interfere with other paths since other paths might be calculated before #callback is called *
			}*/
			
			//Node.activePath = this;
			NodeRun startRNode = startNode.GetNodeRun (runData);
			
			if (searchLength <= 0) {
				Trace (startRNode);
				foundEnd = true;
				return;
			}
			
			startRNode.pathID = pathID;
			startRNode.parent = null;
			startRNode.cost = 0;
			startRNode.g = startNode.penalty;
			startNode.UpdateH (hTarget,heuristic,heuristicScale, startRNode);
			
			/*if (recalcStartEndCosts) {
				startNode.InitialOpen (open,hTarget,startIntPoint,this,true);
			} else {*/
				startNode.Open (runData,startRNode,hTarget,this);
			//}
			
			searchedNodes++;
			
			//any nodes left to search?
			if (runData.open.numberOfItems <= 1) {
				LogError ("No open points, the start node didn't open any nodes");
				return;
			}
			
			currentR = runData.open.Remove ();
		}
		
		public override void CalculateStep (long targetTick) {
			
			int counter = 0;
			
			//Continue to search while there hasn't ocurred an error and the end hasn't been found
			while (!foundEnd && !error) {
				
				//@Performance Just for debug info
				searchedNodes++;
				
				//Close the current node, if the current node is the target node then the path is finnished
				if (currentR.g >= searchLength) {
					
					if (chosenNodeR == null) {
						chosenNodeR = currentR;
					} else if (rnd.NextDouble () < replaceChance) {
						chosenNodeR = currentR;
					}
					
					if (currentR.g >= searchLength+spread) {
						foundEnd = true;
						break;
					}
				} else if (currentR.g > maxGScore) {
					maxGScore = (int)currentR.g;
					maxGScoreNodeR = currentR;
				}
				
				//Loop through all walkable neighbours of the node and add them to the open list.
				currentR.node.Open (runData,currentR, hTarget,this);
				
				//any nodes left to search?
				if (runData.open.numberOfItems <= 1) {
					if (chosenNodeR != null) {
						foundEnd = true;
					} else if (maxGScoreNodeR != null) {
						chosenNodeR = maxGScoreNodeR;
						foundEnd = true;
					} else {
						error = true;
					}
					break;
				}
				
				//Select the node with the lowest F score and remove it from the open list
				currentR = runData.open.Remove ();
				
				//Check for time every 500 nodes, roughly every 0.5 ms usually
				if (counter > 500) {
					
					//Have we exceded the maxFrameTime, if so we should wait one frame before continuing the search since we don't want the game to lag
					if (System.DateTime.Now.Ticks >= targetTick) {
						
						//Return instead of yield'ing, a separate function handles the yield (CalculatePaths)
						return;
					}
					
					counter = 0;
				}
				
				counter++;
			
			}
			
			if (foundEnd && !error) {
				Trace (chosenNodeR);
			}
			
			//Return instead of yielding, a separate function handles the yield (CalculatePaths or CalculatePathsThreaded in AstarPath.cs)
			return;
		}
		
	}
}

