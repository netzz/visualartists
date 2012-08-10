
using UnityEngine;

// Will currently only work with Unity 3.5 Public Beta
public class Webcam : MonoBehaviour
{
    private WebCamTexture webcamTexture;
    
    void Start()
    {
        webcamTexture = new WebCamTexture(640, 480, 30);
		// webcamTexture.requestedFPS=100;
        renderer.material.mainTexture = webcamTexture;
		
        webcamTexture.Play();
		
   }

    void OnGUI()
    {
		webcamTexture.requestedFPS=30;
        if (webcamTexture.isPlaying)
        {
            if (GUILayout.Button("Pause"))
            {
                webcamTexture.Pause();
            }
            if (GUILayout.Button("Stop"))
            {
                webcamTexture.Stop();
            }
        }
        else
        {
            if (GUILayout.Button("Play"))
            {
                webcamTexture.Play();
            }
        }
    }
}