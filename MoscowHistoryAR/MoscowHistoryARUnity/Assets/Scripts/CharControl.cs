using UnityEngine;
using System.Collections;

public class CharControl : MonoBehaviour {
	
	public Transform [] targets;
	
	public Transform livePlane;
	
	public Vector3 custPos;
	
	void Start () {
		
		// Находим target всех персонажей и заносим в массив
		targets=new Transform [transform.childCount];
		int cnt=0;
		foreach (Transform character in transform)
		{
			foreach (Transform target in character)
			{
				if (target.name=="Target")
				{
					targets[cnt++]=target;
				}
			}
		}

	}
	
	// Update is called once per frame
	void OnGUI () {
		if (Event.current.type == EventType.MouseDown)
		{
			//targets[0].position+=new Vector3(2,0,0);
			foreach (Transform target in targets)
			{
				Vector3 randPos=new Vector3(Random.Range(-2.2f, 2.2f), 0, Random.Range(0.0f,4f));
				Vector3 newPos=livePlane.TransformPoint(randPos);
				target.position=newPos;
			}
			
		}		
		
	}
}
