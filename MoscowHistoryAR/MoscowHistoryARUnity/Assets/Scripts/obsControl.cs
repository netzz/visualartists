using UnityEngine;
using System.Collections;
using System.Collections.Generic;


public class obsControl : MonoBehaviour {

	public GameObject mainCamera;
	public LayerMask mask;
	
	
	// Obstacle's prefab
	public GameObject obstacle;
    // Marker's prefab
    public GameObject marker;
    public GameObject procMeshInstance;

    public GameObject plane1;
    public Transform livePlane;


	// Use this for initialization
	bool meshCreated=false;
	
	public float scale = 10f, speed=1f;
	
	Vector3 [] baseHeight;

    public Vector2[] mouse4;
    public int cornersCount;

	public float maxAngle = 45.0f;
	
	void Start () {
        mouse4 = new Vector2[4];
        cornersCount = 0;
		if (mainCamera==null)
		{
			print ("Camera not found");
			return;
		}
		
	
	}
	
// 257, 511
// 305, 505
// 245, 591
// 314, 591
	
	// Update is called once per frame
	void OnGUI () {

        if (Event.current.type == EventType.MouseDown && Event.current.clickCount == 2)
        {
            mouse4[cornersCount] = Input.mousePosition;
            if (++cornersCount > 3)
            {
                //plane1.active = true;
                GameObject newObstacle = Instantiate(procMeshInstance) as GameObject;
                cubeupdate obstacleScript = newObstacle.GetComponent<cubeupdate>();
                
                Vector3 [] worldCoords = new Vector3[4];
                Vector3[] localLivePlaneCoords = new Vector3[4];
                for (int i = 0; i < 4; i++)
                {
			        RaycastHit hit;
                    if (Physics.Raycast(mainCamera.camera.ScreenPointToRay(mouse4[i]), out hit, Mathf.Infinity, mask))
                    {
                        worldCoords[i] = hit.point;
                        localLivePlaneCoords[i] = livePlane.TransformPoint(hit.point);
                        localLivePlaneCoords[i].y += 5f;
                        // print("Original: " + hit.point.x + ", local: " + localLivePlaneCoords[i].x + ", inverse: "+livePlane.InverseTransformPoint(localLivePlaneCoords[i]).x);
                       // localLivePlaneCoords[i].z = 1;
                        //GameObject newObstacle = Instantiate(marker) as GameObject;
                        //newObstacle.transform.position = hit.point;
                    }
                }
                print("Original: " + worldCoords[0].x);
                obstacleScript.cube1[1] = worldCoords[0];
                obstacleScript.cube1[5] = worldCoords[1];
                obstacleScript.cube1[4] = worldCoords[2];
                obstacleScript.cube1[0] = worldCoords[3];
                /*
                plane1.active = false;
                for (int i = 0; i < 4; i++)
                {
                    RaycastHit hit;
                    if (Physics.Raycast(mainCamera.camera.ScreenPointToRay(mouse4[i]), out hit, Mathf.Infinity, mask))
                    {
                        worldCoords[i] = hit.point;
                        //GameObject newObstacle = Instantiate(marker) as GameObject;
                        //newObstacle.transform.position = hit.point;

                    }
                }
                 */
               
                print("inverse: " + livePlane.InverseTransformPoint(localLivePlaneCoords[0]).x);
                obstacleScript.cube1[3] = livePlane.InverseTransformPoint(localLivePlaneCoords[0]);
                obstacleScript.cube1[7] = livePlane.InverseTransformPoint(localLivePlaneCoords[1]);
                obstacleScript.cube1[6] = livePlane.InverseTransformPoint(localLivePlaneCoords[2]);
                obstacleScript.cube1[2] = livePlane.InverseTransformPoint(localLivePlaneCoords[3]);
                
                /*
                obstacleScript.cube1[3] = livePlane.TransformPoint(localLivePlaneCoords[0]);
                obstacleScript.cube1[7] = livePlane.TransformPoint(localLivePlaneCoords[1]);
                obstacleScript.cube1[6] = livePlane.TransformPoint(localLivePlaneCoords[2]);
                obstacleScript.cube1[2] = livePlane.TransformPoint(localLivePlaneCoords[3]);
                */
                newObstacle.collider.isTrigger = false;
                obstacleScript.UpdateMesh();
                AstarPath.active.UpdateGraphs(newObstacle.collider.bounds);
                //AstarPath.active.AutoScan();
                newObstacle.collider.isTrigger = true;

                cornersCount = 0;
            }
            
        }	
        /*
		if (Event.current.type == EventType.MouseDown && Event.current.clickCount == 2)
		{
			
			// Mesh mesh=new Mesh();
			
			RaycastHit hit;
			if (Physics.Raycast	(mainCamera.camera.ScreenPointToRay (Input.mousePosition), out hit, Mathf.Infinity, mask)) 
			{
				
				//transform.position=hit.point;
				GameObject newObstacle = Instantiate (obstacle) as GameObject;
				newObstacle.transform.position=hit.point;
				newObstacle.collider.isTrigger=false;
				
				AstarPath.active.UpdateGraphs(newObstacle.collider.bounds);
				AstarPath.active.AutoScan();
				newObstacle.collider.isTrigger=true;
				//AstarPath.active.Scan();
				
			}
			
		}
		*/
	}
}


