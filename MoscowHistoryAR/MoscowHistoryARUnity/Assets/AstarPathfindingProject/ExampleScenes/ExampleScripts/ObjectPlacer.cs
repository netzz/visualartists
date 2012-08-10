using UnityEngine;
using System.Collections;

/** Small sample script for placing obstacles */
public class ObjectPlacer : MonoBehaviour {
	
	public GameObject go; /** GameObject to place. Make sure the layer it is in is included in the collision mask on the GridGraph settings (assuming a GridGraph) */
	public bool direct = false; /** Flush Graph Updates directly after placing. Slower, but updates are applied immidiately */
	
	// Update is called once per frame
	void Update () {
		
		if (Input.GetKeyDown ("p")) {
			PlaceObject ();
		}
	}
	
	public void PlaceObject () {
		
		Ray ray = Camera.main.ScreenPointToRay (Input.mousePosition);
		RaycastHit hit;
		if ( Physics.Raycast (ray, out hit, Mathf.Infinity)) {
			Vector3 p = hit.point;
			
			GameObject obj = (GameObject)GameObject.Instantiate (go,p,Quaternion.identity);
			
			Bounds b = obj.collider.bounds;
			//Pathfinding.Console.Write ("// Placing Object\n");
			AstarPath.active.UpdateGraphs (b);
			if (direct) {
				//Pathfinding.Console.Write ("// Flushing\n");
				AstarPath.active.FlushGraphUpdates();
			}
		}
	}
}
