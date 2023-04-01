using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RigController : MonoBehaviour {
	public GameObject hip;
	public GameObject chest;
	public GameObject head;
	public GameObject shoulder_l;
	public GameObject shoulder_r;
	public GameObject elbow_l;
	public GameObject elbow_r;
	public GameObject hand_l;
	public GameObject hand_r;
	public GameObject knee_l;
	public GameObject knee_r;
	public GameObject foot_l;
	public GameObject foot_r;
    
	public LineRenderer torso;
	public LineRenderer arms;
	public LineRenderer legs;
	private List<Vector3> torsoPoints;
	private List<Vector3> armPoints;
	private List<Vector3> legPoints;
    
    private MessageQueueHandler _messageQueueHandler;
    
	void Start () 
    {
        _messageQueueHandler = MessageQueueHandler.getInstance();
        
		torsoPoints = new List<Vector3>();
		armPoints = new List<Vector3>();
		legPoints = new List<Vector3>();	
        
		//torso.widthMultiplier = 0.04f;
		//legs.widthMultiplier = 0.04f;
		//arms.widthMultiplier = 0.04f;
        
		torso.material.color = Color.green;
		legs.material.color = Color.magenta;
		arms.material.color = Color.red;
        
		torso.enabled = true;
		legs.enabled = true;
		arms.enabled = true;
	}
	
	void Update () 
    {
        Dictionary<int, V3> modelPoints = new Dictionary<int, V3>();
        bool modelUpdated = 
            _messageQueueHandler.getModelPointsIfChanged(ref modelPoints);
        
        if (modelUpdated)
        {
            hip.transform.position = MimikryMath.V3toVector3(modelPoints[0]);
            chest.transform.position = MimikryMath.V3toVector3(modelPoints[1]);
            head.transform.position = MimikryMath.V3toVector3(modelPoints[2]);
            shoulder_l.transform.position = MimikryMath.V3toVector3(modelPoints[3]);
            shoulder_r.transform.position = MimikryMath.V3toVector3(modelPoints[4]);
            elbow_l.transform.position = MimikryMath.V3toVector3(modelPoints[5]);
            elbow_r.transform.position = MimikryMath.V3toVector3(modelPoints[6]);
            hand_l.transform.position = MimikryMath.V3toVector3(modelPoints[7]);
            hand_r.transform.position = MimikryMath.V3toVector3(modelPoints[8]);
            knee_l.transform.position = MimikryMath.V3toVector3(modelPoints[9]);
            knee_r.transform.position = MimikryMath.V3toVector3(modelPoints[10]);
            foot_l.transform.position = MimikryMath.V3toVector3(modelPoints[11]);
            foot_r.transform.position = MimikryMath.V3toVector3(modelPoints[12]);
            
            torsoPoints.Clear();
            armPoints.Clear();
            legPoints.Clear();
            
            torsoPoints.Add(head.transform.position);
            torsoPoints.Add(chest.transform.position);
            torsoPoints.Add(hip.transform.position);
            
            legPoints.Add(foot_l.transform.position);
            legPoints.Add(knee_l.transform.position);
            legPoints.Add(hip.transform.position);
            legPoints.Add(knee_r.transform.position);
            legPoints.Add(foot_r.transform.position);
            
            armPoints.Add(hand_l.transform.position);
            armPoints.Add(elbow_l.transform.position);
            armPoints.Add(shoulder_l.transform.position);
            armPoints.Add(chest.transform.position);
            armPoints.Add(shoulder_r.transform.position);
            armPoints.Add(elbow_r.transform.position);
            armPoints.Add(hand_r.transform.position);
            
            torso.positionCount = torsoPoints.Count;
            legs.positionCount = legPoints.Count;
            arms.positionCount	= armPoints.Count;
            
            torso.SetPositions(torsoPoints.ToArray());
            legs.SetPositions(legPoints.ToArray());
            arms.SetPositions(armPoints.ToArray());
        }
	}
}
