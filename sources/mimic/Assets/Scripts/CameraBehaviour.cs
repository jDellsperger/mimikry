using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraBehaviour : MonoBehaviour {
    
	public bool lookAtRig;
	public GameObject rig;
	public int distanceLowCap = 200;
	public float zoomSpeed = 100;
    
	private Visualizer _visualizer;
    private Transform _intersectionsCOM;
    
    // Use this for initialization
    void Start () {
		lookAtRig = false;
		_visualizer = (Visualizer) rig.GetComponent(typeof(Visualizer));	
	}
	
	// Update is called once per frame
	void Update () {
		if(Input.GetAxis("Mouse ScrollWheel") != 0)
		{
            if (!_intersectionsCOM)
            {
                _intersectionsCOM = _visualizer.getIntersectionsCOM();
            }
			gameObject.transform.Translate(0,0,Input.GetAxis("Mouse ScrollWheel") * zoomSpeed);
			float distance = Vector3.Distance(_intersectionsCOM.position, gameObject.transform.position);
            
			if(distance < distanceLowCap) 
            {
				gameObject.transform.Translate(0, 0, distance - distanceLowCap);
			}
		}
		
		if (Input.GetKeyDown(KeyCode.B)) 
        {
            _intersectionsCOM = _visualizer.getIntersectionsCOM();
			lookAtRig = !lookAtRig;
		}
        
		if(lookAtRig) 
        {
			gameObject.transform.LookAt(_intersectionsCOM);
		}
        
		if(Input.GetMouseButton(0)) 
        {
            if (!_intersectionsCOM)
            {
                _intersectionsCOM = _visualizer.getIntersectionsCOM();
            }
			gameObject.transform.RotateAround(_intersectionsCOM.position, Vector3.up, Input.GetAxis("Mouse X"));
			gameObject.transform.RotateAround(_intersectionsCOM.position, gameObject.transform.right, -Input.GetAxis("Mouse Y"));
		}
	}
}
