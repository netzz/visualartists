using UnityEngine;
using System.Collections;

public class cubeupdate : MonoBehaviour {
    public float scale = 1f;
    public float speed = 0f;
    public bool recalculateNormals = true;
    public Vector3[] cube1;
    private Vector3[] _baseVertices;
    private Mesh _mesh;
    static private System.Random _rnd = new System.Random(999);
    public bool updateRealtime=false;
    Vector3[] cube;
    MeshCollider mc ;

	void Start () {
        _mesh = GetComponent<MeshFilter>().mesh;
        mc = GetComponent<MeshCollider>();
        cube = new Vector3[8];
        /*
        cube[0] = new Vector3(-1.0f, 0.0f, 0.0f);
        cube[1] = new Vector3(-1.0f, 0.0f, 1.0f);
        cube[2] = new Vector3(-1.0f, 1.0f, 0.0f);
        cube[3] = new Vector3(-1.0f, 1.0f, 1.0f);
        cube[4] = new Vector3(0.0f, 0.0f, 0.0f);
        cube[5] = new Vector3(0.0f, 0.0f, 1.0f);
        cube[6] = new Vector3(0.0f, 1.0f, 0.0f);
        cube[7] = new Vector3(0.0f, 1.0f, 1.0f);
        */

	}
	
	// Update is called once per frame
	void Update () {
        if (updateRealtime == true)
            UpdateMesh();
	}

    public void UpdateMesh()
    {
        _mesh = GetComponent<MeshFilter>().mesh;
        mc = GetComponent<MeshCollider>();
        cube = new Vector3[8];

        mc.convex = false;
        if (_baseVertices == null)
        {
            _baseVertices = _mesh.vertices;
        }//end if
        Vector3[] vertices = new Vector3[_baseVertices.Length];
        cube = cube1;
        vertices[0] = cube[4];
        vertices[1] = cube[5];
        vertices[2] = cube[6];
        vertices[3] = cube[7];
        vertices[4] = cube[0];
        vertices[5] = cube[1];
        vertices[6] = cube[2];
        vertices[7] = cube[3];
        vertices[8] = cube[4];
        vertices[9] = cube[6];
        vertices[10] = cube[6];
        vertices[11] = cube[2];
        vertices[12] = cube[7];
        vertices[13] = cube[0];
        vertices[14] = cube[3];
        vertices[15] = cube[2];
        vertices[16] = cube[4];
        vertices[17] = cube[1];
        vertices[18] = cube[0];
        vertices[19] = cube[5];
        vertices[20] = cube[5];
        vertices[21] = cube[3];
        vertices[22] = cube[1];
        vertices[23] = cube[7];
        /*
        for (int i = 0; i < vertices.Length; i++)
        {
            Vector3 vertex = _baseVertices[i];
            print("n: " + i + ", x:" + (vertex.x + 1) + ", y:" + vertex.y + ", z:" + vertex.z);
            for (int n = 0; n < cube.Length; n++)
            {
                if (cube[n].x == (vertex.x+1) && cube[n].y == vertex.y && cube[n].z == vertex.z)
                {
                    print("Found: "+n);
                }
            }


            //update
            vertices[i] = vertex;
        }
         * */
        _mesh.vertices = vertices;

        mc.sharedMesh = _mesh;
        if (recalculateNormals)
        {
            _mesh.RecalculateNormals();
        }//end if
        _mesh.RecalculateBounds();
        mc.convex = true;
    }

}
