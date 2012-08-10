using System;
using UnityEngine;
using Pathfinding;

namespace Pathfinding {
	/** Returns a path heading away from a specified point to avoid. The search will terminate when G \>= \a length (passed to the constructor) + FleePath::spread.\n
	 * \ingroup paths
	 * Can be used to make an AI to flee from an enemy (cannot guarantee that it will not be forced into corners though :D )\n
	 * \code

//Call a FleePath call like this, assumes that a Seeker is attached to the GameObject
Vector3 thePointToFleeFrom = Vector3.zero;

//The path will be returned when the path is over a specified length (or more accurately has "costed" more than a specific value)
//This is usally roughly the distance from the start to the end multiplied by 100
int theGScoreToStopAt = 1000;

//Create a path object
FleePath path = new FleePath  (transform.position, thePointToFleeFrom, theGScoreToStopAt);

//Get the Seeker component which must be attached to this GameObject
Seeker seeker = GetComponent<Seeker>();

//Start the path and return the result to MyCompleteFunction (which is a function you have to define, the name can of course be changed)
seeker.StartPath (path,MyCompleteFunction);

	 * \endcode
	 * \astarpro */
	public class FleePath : RandomPath {
		
		public float fleeStrength = 2;
		
		public FleePath (Vector3 start, Vector3 avoid, int length, OnPathDelegate callbackDelegate = null) : base (start,length,callbackDelegate) {
			
			/*if (AstarPath.active.heuristicScale == 0) {
				heuristicScale = -1;
			} else {
				heuristicScale = System.Math.Abs (AstarPath.active.heuristicScale) * -1;
			}*/
			
			originalEndPoint = avoid;
			
			endPoint = avoid;
			searchLength = length;
			hTarget = (Int3)avoid;
		}
		
		
		public override void Prepare () {
			base.Prepare ();
			//The base.Prepare function changes these variables, so we need to change it back again
			endPoint = originalEndPoint;
			hTarget = (Int3)originalEndPoint;
			heuristicScale = -fleeStrength;
			
			if (AstarPath.active.heuristic != Heuristic.None) {
				heuristic = AstarPath.active.heuristic;
			} else {
				heuristic = Heuristic.Euclidean;
			}
		}
	
	}
}
