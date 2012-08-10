using UnityEngine;
using System.Collections;
using System;
using System.Runtime.InteropServices;

public class PluginImport : MonoBehaviour {

	
	[DllImport ("ASimplePlugin")]
    private static extern int UpdateNI([In, Out] ushort[] data);

    [DllImport("ASimplePlugin")]
    private static extern string StartNI();

    [DllImport("ASimplePlugin")]
    private static extern string StopNI();
    private Texture2D m_Texture;
    private int wdth = 320;
    private int hght = 240;
    public int thr = 3000;

	void Start () {
        m_Texture = new Texture2D(wdth, hght, TextureFormat.ARGB32, false);
        // Make sure that the texture name exists etc.
        m_Texture.Apply();
        // Assign texture to the renderer
        if (renderer)
        {
            renderer.material.mainTexture = m_Texture;
        }
        else
        {
            Debug.Log("Game object has no renderer to assign the generated texture to!");
        }

		Debug.Log(StartNI());
	}
    void Update()    {
        ushort [] res=new ushort[wdth*hght];


        Color[] tmp = new Color[wdth * hght];

        


        int r=UpdateNI(res);
//        print(res[100]);
        for (int j = 0; j < hght; j++)
        {
            for (int i = 0; i < wdth; i++)
            {
                if (res[i + j * wdth] < thr && res[i + j * wdth]!=0)
                {
                    tmp[i + j * wdth].a = 1f - (float)Convert.ToInt16(res[i + j * wdth]) / 10000;
                }
                else
                {
                    tmp[i + j * wdth].a = 0;
                }

                tmp[i + j * wdth].r = 1f-(float)Convert.ToInt16(res[i + j * wdth]) / 10000;
                tmp[i + j * wdth].g = 1f-(float)Convert.ToInt16(res[i + j * wdth]) / 10000;
                tmp[i + j * wdth].b = 1f-(float)Convert.ToInt16(res[i + j * wdth]) / 10000;

                // rgb implementation										
                //tmp[i+j*wdth].a = 1.0f;
                //tmp[i+j*wdth].r = (float)Convert.ToInt16(jitData[(wdth-1-i+j*wdth)*3])/255;
                //tmp[i+j*wdth].g = (float)Convert.ToInt16(jitData[1+(wdth-1-i+j*wdth)*3])/255;
                //tmp[i+j*wdth].b = (float)Convert.ToInt16(jitData[2+(wdth-1-i+j*wdth)*3])/255;
            }
        }

        m_Texture.SetPixels(tmp, 0);
        m_Texture.Apply(false);
        //print(res[1]);
    }
    void OnDesroy()
    {
        StopNI();
    }

}
