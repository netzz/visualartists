using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Pathfinding
{
	/** Floods the area completely for easy computation of any path to a single point.
This path is a bit special, because it does not do anything useful by itself. What it does is that it calculates paths to all nodes it can reach, floods it.
This data will remain stored in the graph. Then you can call a FloodPathTracer path, that path will trace the path from it's starting point all the way to where this path started flooding and thus generating a path extreamly quicly.\n
It is very useful in for example TD (Tower Defence) games where all your AIs will walk to the same point, but from different places, and you do not update the graph or change the target point very often,
what changes is their positions and new AIs spawn all the time (which makes it hard to use the MultiTargetPath).\n

With this path type, it can all be handled easily.
- At start, you simply start ONE FloodPath and save the reference (it will be needed later).
- Then when a unit is spawned or needs it's path recalculated, start a FloodPathTracer path from it's position.
   It will then find the shortest path to the point specified when you called the FloodPath extreamly quickly.
- If you update the graph (for example place a tower in a TD game) or need to change the target point, you simply call a new FloodPath (and store it's reference).
 
\warning Best to mention this early. When using this path, you should not call ANY OTHER PATHS (except FloodPathTracer) for it to work correctly.
	 Though, if you are sure two areas will not connect to each other, you can actually call other paths in the other area, but be careful.\n
	 What could happen if another path is called in the same area as this one is that it overwrites the pathID and more importantly the data for the paths,
	 which would render FloodPathTracer paths invalid, or in the worst case, get stuck in large loops and slow the game down a lot.\n
	 Sorry about the shouting, but hopefully I will not get the forums filled with threads about people using this path type alongside other path types now anyway.
	
Here follows some example code of the above list of steps:
\code
public static FloodPath fpath;

public void Start () {
	fpath = new FloodPath (someTargetPosition, null);
	AstarPath.StartPath (fpath);
}
\endcode

When searching for a new path to \a someTargetPosition from let's say \a transform.position, you do
\code
FloodPathTracer fpathTrace = new FloodPathTracer (transform.position,fpath,null);
seeker.StartPath (fpathTrace,OnPathComplete);
\endcode
Where OnPathComplete is your callback function.

\note This path type relies on pathIDs being stored in the graph, but pathIDs are only 16 bits, meaning they will overflow after 65536 paths.
When that happens all pathIDs in the graphs will be cleared, so at that point you will also need to recalculate the FloodPath.\n
To do so, register to the AstarPath::On65KOverflow callback:
\code
public void Start () {
	AstarPath.On65KOverflow += MyCallbackFunction;
}

public void MyCallbackFunction () {
	//The callback is nulled every time it is called, so we need to register again
	AstarPath.On65KOverflow += MyCallbackFunction;
	
	//Recalculate the path
} \endcode 
This will happen after a very long time into the game, but it will happen eventually (look at the 'path number' value on the log messages when paths are completed for a hint about when)
\n
\n
Anothing thing to note is that if you are using NNConstraints on the FloodPathTracer, they must always inherit from Pathfinding::PathIDConstraint.\n
The easiest is to just modify the instance of PathIDConstraint which is created as the default one.

\astarpro

\shadowimage{floodPathExample.png}

\ingroup paths

*/
	public class FloodPath : Path
	{
		
		/** Creates a new FloodPath instance */
		public FloodPath (Vector3 start, OnPathDelegate callbackDelegate) : base (start,Vector3.zero,callbackDelegate) {
			hasEndPoint = false;
			heuristic = Heuristic.None;
		}
		
		/** Opens nodes until there are none left to search (or until the max time limit has been exceeded) */
		public override void CalculateStep (long targetTick) {
			
			int counter = 0;
			
			//Continue to search while there hasn't ocurred an error and the end hasn't been found
			while (!foundEnd && !error) {
				
				//@Performance Just for debug info
				searchedNodes++;
				
				//Close the current node, if the current node is the target node then the path is finnished
				if (currentR.node == endNode) {
					foundEnd = true;
					break;
				}
				
				//Loop through all walkable neighbours of the node and add them to the open list.
				currentR.node.Open (runData,currentR, hTarget,this);
				
				//any nodes left to search?
				if (runData.open.numberOfItems <= 1) {
					foundEnd = true;
					return;
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
		}
	}
}

