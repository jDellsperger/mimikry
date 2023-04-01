using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ModelRigController : MonoBehaviour 
{
    
    struct CoordinateSystem
    {
        public Vector3 o, x, y, z;
        public Matrix4x4 toWorld;
        public Matrix4x4 fromWorld;
    };
    
	public Transform root;
	public Transform hip;
	public Transform chest;
	public Transform head;
	public Transform shoulder_l;
	public Transform shoulder_r;
	public Transform elbow_l;
	public Transform elbow_r;
	public Transform hand_l;
	public Transform hand_r;
    public Transform hip_l;
    public Transform hip_r;
	public Transform knee_l;
	public Transform knee_r;
	public Transform foot_l;
	public Transform foot_r;
    
    public bool showCoordinateSystem = false;
    public bool showModelPoints = false;
    
    private MessageQueueHandler _messageQueueHandler;
    
    private bool _modelMatched = false;
    
    private Vector3 _lastHipP;
    private Vector3 _lastChestP;
    private Vector3 _lastHeadP;
    private Vector3 _lastShoulderLP;
    private Vector3 _lastShoulderRP;
    private Vector3 _lastElbowLP;
    private Vector3 _lastElbowRP;
    private Vector3 _lastHandLP;
    private Vector3 _lastHandRP;
    
    private Vector3 _lastHipChest;
    private Vector3 _lastChestHead;
    private Vector3 _lastChestShoulderL;
    private Vector3 _lastChestShoulderR;
    private Vector3 _lastShoulderElbowL;
    private Vector3 _lastShoulderElbowR;
    private Vector3 _lastElbowHandL;
    private Vector3 _lastElbowHandR;
    
    private GameObject _coordinateSystem;
    private GameObject[] _pointCoordinateSystems;
    private GameObject _modelPointContainer;
    
    private CoordinateSystem _cChestL;
    private CoordinateSystem _cChestR;
    private CoordinateSystem _cHipL;
    private CoordinateSystem _cHipR;
    
	void Start () 
    {
        _messageQueueHandler = MessageQueueHandler.getInstance();
        _pointCoordinateSystems = new GameObject[13];
	}
	
	void Update () 
    {
        Dictionary<int, V3> modelWorldPoints = new Dictionary<int, V3>();
        bool modelUpdated = 
            _messageQueueHandler.getModelPointsIfChanged(ref modelWorldPoints);
        
        if (modelUpdated)
        {
            // 0. get model points in world coordinates
            Vector3[] worldPoints = new Vector3[13];
            
            for (int i = 0; i < 13; i++)
            {
                worldPoints[i] = MimikryMath.V3toVector3(modelWorldPoints[i]);
            }
            
            Vector3 hipW = worldPoints[0];
            Vector3 chestW = worldPoints[1];
            Vector3 headW = worldPoints[2];
            Vector3 shoulderLW = worldPoints[3];
            Vector3 shoulderRW = worldPoints[4];
            Vector3 elbowLW = worldPoints[5];
            Vector3 elbowRW = worldPoints[6];
            Vector3 handLW = worldPoints[7];
            Vector3 handRW = worldPoints[8];
            Vector3 kneeLW = worldPoints[9];
            Vector3 kneeRW = worldPoints[10];
            Vector3 footLW = worldPoints[11];
            Vector3 footRW = worldPoints[12];
            
            // 1. define bone coordinatesystems
            
            // chest left coordinatesystem
            Vector3 chestLO = chestW;
            Vector3 chestLX = shoulder_l.right;
            Vector3 chestLY = shoulder_l.up;
            Vector3 chestLZ = shoulder_l.forward;
            CoordinateSystem cChestL = 
                initializeCoordinateSystem(chestLO,
                                           chestLX,
                                           chestLY,
                                           chestLZ);
            
            // chest right coordinatesystem
            Vector3 chestRO = chestW;
            Vector3 chestRX = shoulder_r.right;
            Vector3 chestRY = shoulder_r.up;
            Vector3 chestRZ = shoulder_r.forward;
            CoordinateSystem cChestR = 
                initializeCoordinateSystem(chestRO,
                                           chestRX,
                                           chestRY,
                                           chestRZ);
            
            // hip left coordinatesystem
            Vector3 hipLO = hipW;
            Vector3 hipLX = hip_l.right;
            Vector3 hipLY = hip_l.up;
            Vector3 hipLZ = hip_l.forward;
            CoordinateSystem cHipL = 
                initializeCoordinateSystem(hipLO,
                                           hipLX,
                                           hipLY,
                                           hipLZ);
            
            // hip right coordinatesystem
            Vector3 hipRO = hipW;
            Vector3 hipRX = hip_r.right;
            Vector3 hipRY = hip_r.up;
            Vector3 hipRZ = hip_r.forward;
            CoordinateSystem cHipR = 
                initializeCoordinateSystem(hipRO,
                                           hipRX,
                                           hipRY,
                                           hipRZ);
            
            // fix coordinate systems
            if (!_modelMatched)
            {
                /*
                // chest left coordinatesystem
                Vector3 chestLO = chestW;
                Vector3 chestLX = shoulder_l.right;
                Vector3 chestLY = shoulder_l.up;
                Vector3 chestLZ = shoulder_l.forward;
                _cChestL = 
                    initializeCoordinateSystem(chestLO,
                                               chestLX,
                                               chestLY,
                                               chestLZ);
                
                // chest right coordinatesystem
                Vector3 chestRO = chestW;
                Vector3 chestRX = shoulder_r.right;
                Vector3 chestRY = shoulder_r.up;
                Vector3 chestRZ = shoulder_r.forward;
                _cChestR = 
                    initializeCoordinateSystem(chestRO,
                                               chestRX,
                                               chestRY,
                                               chestRZ);
                
                // hip left coordinatesystem
                Vector3 hipLO = hipW;
                Vector3 hipLX = hip_l.right;
                Vector3 hipLY = hip_l.up;
                Vector3 hipLZ = hip_l.forward;
                _cHipL = 
                    initializeCoordinateSystem(hipLO,
                                               hipLX,
                                               hipLY,
                                               hipLZ);
                
                // hip right coordinatesystem
                Vector3 hipRO = hipW;
                Vector3 hipRX = hip_r.right;
                Vector3 hipRY = hip_r.up;
                Vector3 hipRZ = hip_r.forward;
                _cHipR = 
                    initializeCoordinateSystem(hipRO,
                                               hipRX,
                                               hipRY,
                                               hipRZ);
                                               */
            }
            
            // root coordinatesystem
            Vector3 rootY = (shoulderRW - shoulderLW).normalized;
            
            Vector3 SlC = chestW - shoulderLW;
            float cosAlpha = Vector3.Dot(rootY, SlC.normalized);
            float SlSrOffset = cosAlpha * SlC.magnitude;
            Vector3 projectedChest = shoulderLW + (rootY * SlSrOffset);
            Vector3 rootX = (chestW - projectedChest).normalized;
            Vector3 rootO = projectedChest;
            
            Vector3 rootZ = Vector3.Cross(rootX, rootY);
            
            CoordinateSystem cRoot = 
                initializeCoordinateSystem(rootO, rootX, rootY, rootZ);
            
            if (_coordinateSystem != null)
            {
                Destroy(_coordinateSystem);
            }
            
            if (showCoordinateSystem)
            {
                _coordinateSystem = new GameObject();
                _coordinateSystem.name = "Root Coordinate System";
                paintCoordinateSystem(ref _coordinateSystem, cRoot);
            }
            
            // hip coordinatesystem
            Vector3 hipO = hipW;
            Vector3 hipX = (hipW - chestW).normalized;
            Vector3 hipY = rootY;
            Vector3 hipZ = Vector3.Cross(hipX, hipY);
            CoordinateSystem cHip = 
                initializeCoordinateSystem(hipO,
                                           hipX,
                                           hipY,
                                           hipZ);
            
            if (_pointCoordinateSystems[0] != null)
            {
                Destroy(_pointCoordinateSystems[0]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[0] = new GameObject();
                _pointCoordinateSystems[0].name = "Hip Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[0], cHip);
            }
            
            // shoulder left coordinatesystem
            Vector3 shoulderLO = shoulderLW;
            Vector3 shoulderLX = (shoulderLW - elbowLW).normalized;
            Vector3 shoulderLY = rootZ;
            Vector3 shoulderLZ = Vector3.Cross(shoulderLX, shoulderLY);
            CoordinateSystem cShoulderL = 
                initializeCoordinateSystem(shoulderLO,
                                           shoulderLX,
                                           shoulderLY,
                                           shoulderLZ);
            
            if (_pointCoordinateSystems[3] != null)
            {
                Destroy(_pointCoordinateSystems[3]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[3] = new GameObject();
                _pointCoordinateSystems[3].name = "Shoulder Left Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[3], cShoulderL);
            }
            
            // elbow left coordinatesystem
            Vector3 elbowLO = elbowLW;
            Vector3 elbowLX = (elbowLW - handLW).normalized;
            Vector3 elbowLY = Vector3.Cross(shoulderLX, elbowLX);
            Vector3 elbowLZ = Vector3.Cross(elbowLX, elbowLY);
            CoordinateSystem cElbowL = 
                initializeCoordinateSystem(elbowLO,
                                           elbowLX,
                                           elbowLY,
                                           elbowLZ);
            
            if (_pointCoordinateSystems[5] != null)
            {
                Destroy(_pointCoordinateSystems[5]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[5] = new GameObject();
                _pointCoordinateSystems[5].name = "Elbow Left Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[5], cElbowL);
            }
            
            // shoulder right coordinatesystem
            Vector3 shoulderRO = shoulderRW;
            Vector3 shoulderRX = (shoulderRW - elbowRW).normalized;
            Vector3 shoulderRZ = shoulderLZ;
            Vector3 shoulderRY = Vector3.Cross(shoulderRZ, shoulderRX);
            CoordinateSystem cShoulderR = 
                initializeCoordinateSystem(shoulderRO,
                                           shoulderRX,
                                           shoulderRY,
                                           shoulderRZ);
            
            if (_pointCoordinateSystems[4] != null)
            {
                Destroy(_pointCoordinateSystems[4]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[4] = new GameObject();
                _pointCoordinateSystems[4].name = "Shoulder Right Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[4], cShoulderR);
            }
            
            // elbow right coordinatesystem
            Vector3 elbowRO = elbowRW;
            Vector3 elbowRX = (elbowRW - handRW).normalized;
            Vector3 elbowRY = Vector3.Cross(shoulderRX, elbowRX);
            Vector3 elbowRZ = Vector3.Cross(elbowRX, elbowRY);
            CoordinateSystem cElbowR = 
                initializeCoordinateSystem(elbowRO,
                                           elbowRX,
                                           elbowRY,
                                           elbowRZ);
            
            if (_pointCoordinateSystems[6] != null)
            {
                Destroy(_pointCoordinateSystems[6]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[6] = new GameObject();
                _pointCoordinateSystems[6].name = "Elbow Right Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[6], cElbowR);
            }
            
            // thigh left coordinatesystem
            Vector3 thighLO = hipW + (cHipL.x * ((shoulderLW - rootO).magnitude));
            Vector3 thighLX = (thighLO - kneeLW).normalized;
            Vector3 thighLY = hipY;
            Vector3 thighLZ = Vector3.Cross(thighLX, thighLY);
            CoordinateSystem cThighL = 
                initializeCoordinateSystem(thighLO,
                                           thighLX,
                                           thighLY,
                                           thighLZ);
            
            if (_pointCoordinateSystems[9] != null)
            {
                Destroy(_pointCoordinateSystems[9]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[9] = new GameObject();
                _pointCoordinateSystems[9].name = "Thigh Left Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[9], cThighL);
            }
            
            // thigh right coordinatesystem
            Vector3 thighRO = hipW + (cHipL.x * -((shoulderLW - rootO).magnitude));
            Vector3 thighRX = (thighRO - kneeRW).normalized;
            Vector3 thighRY = hipY;
            Vector3 thighRZ = Vector3.Cross(thighRX, thighRY);
            CoordinateSystem cThighR = 
                initializeCoordinateSystem(thighRO,
                                           thighRX,
                                           thighRY,
                                           thighRZ);
            
            if (_pointCoordinateSystems[10] != null)
            {
                Destroy(_pointCoordinateSystems[10]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[10] = new GameObject();
                _pointCoordinateSystems[10].name = "Thigh Right Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[10], cThighR);
            }
            
            // shin left coordinatesystem
            Vector3 shinLO = kneeLW;
            Vector3 shinLX = (kneeLW - footLW).normalized;
            Vector3 shinLY = Vector3.Cross(shinLX, thighLX);
            Vector3 shinLZ = Vector3.Cross(shinLX, shinLY);
            CoordinateSystem cShinL = 
                initializeCoordinateSystem(shinLO,
                                           shinLX,
                                           shinLY,
                                           shinLZ);
            
            if (_pointCoordinateSystems[11] != null)
            {
                Destroy(_pointCoordinateSystems[11]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[11] = new GameObject();
                _pointCoordinateSystems[11].name = "Shin Left Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[11], cShinL);
            }
            
            // shin right coordinatesystem
            Vector3 shinRO = kneeRW;
            Vector3 shinRX = (kneeRW - footRW).normalized;
            Vector3 shinRY = Vector3.Cross(shinRX, thighRX);
            Vector3 shinRZ = Vector3.Cross(shinRX, shinRY);
            CoordinateSystem cShinR = 
                initializeCoordinateSystem(shinRO,
                                           shinRX,
                                           shinRY,
                                           shinRZ);
            
            if (_pointCoordinateSystems[12] != null)
            {
                Destroy(_pointCoordinateSystems[12]);
            }
            
            if (showCoordinateSystem)
            {
                _pointCoordinateSystems[12] = new GameObject();
                _pointCoordinateSystems[12].name = "Shin Right Coordinate System";
                paintCoordinateSystem(ref _pointCoordinateSystems[12], cShinR);
            }
            
            // 2. calculate rotations relative to parent
            
            // hip rotation
            Matrix4x4 rRw = cRoot.toWorld;
            rRw[0,3] = 0;
            rRw[1,3] = 0;
            rRw[2,3] = 0;
            Matrix4x4 wRh = cHip.fromWorld;
            wRh[0,3] = 0;
            wRh[1,3] = 0;
            wRh[2,3] = 0;
            Matrix4x4 rRh = rRw * wRh;
            hip.localRotation = rRh.rotation;
            
            // shoulder rotation left
            Matrix4x4 clRw = cChestL.toWorld;
            clRw[0,3] = 0;
            clRw[1,3] = 0;
            clRw[2,3] = 0;
            Matrix4x4 wRsl = cShoulderL.fromWorld;
            wRsl[0,3] = 0;
            wRsl[1,3] = 0;
            wRsl[2,3] = 0;
            Matrix4x4 clRsl = clRw * wRsl;
            elbow_l.localRotation = clRsl.rotation;
            
            // elbow rotation left
            Matrix4x4 slRw = cShoulderL.toWorld;
            slRw[0,3] = 0;
            slRw[1,3] = 0;
            slRw[2,3] = 0;
            Matrix4x4 wRel = cElbowL.fromWorld;
            wRel[0,3] = 0;
            wRel[1,3] = 0;
            wRel[2,3] = 0;
            Matrix4x4 slRel = slRw * wRel;
            hand_l.localRotation = slRel.rotation;
            
            // shoulder rotation right
            Matrix4x4 crRw = cChestR.toWorld;
            crRw[0,3] = 0;
            crRw[1,3] = 0;
            crRw[2,3] = 0;
            Matrix4x4 wRsr = cShoulderR.fromWorld;
            wRsr[0,3] = 0;
            wRsr[1,3] = 0;
            wRsr[2,3] = 0;
            Matrix4x4 crRsr = crRw * wRsr;
            elbow_r.localRotation = crRsr.rotation;
            
            // elbow rotation right
            Matrix4x4 srRw = cShoulderR.toWorld;
            srRw[0,3] = 0;
            srRw[1,3] = 0;
            srRw[2,3] = 0;
            Matrix4x4 wRer = cElbowR.fromWorld;
            wRer[0,3] = 0;
            wRer[1,3] = 0;
            wRer[2,3] = 0;
            Matrix4x4 srRer = srRw * wRer;
            hand_r.localRotation = srRer.rotation;
            
            // thigh rotation left
            Matrix4x4 hlRw = cHipL.toWorld;
            hlRw[0,3] = 0;
            hlRw[1,3] = 0;
            hlRw[2,3] = 0;
            Matrix4x4 wRtl = cThighL.fromWorld;
            wRtl[0,3] = 0;
            wRtl[1,3] = 0;
            wRtl[2,3] = 0;
            Matrix4x4 hlRtl = hlRw * wRtl;
            knee_l.localRotation = hlRtl.rotation;
            
            // thigh rotation right
            Matrix4x4 hrRw = cHipR.toWorld;
            hrRw[0,3] = 0;
            hrRw[1,3] = 0;
            hrRw[2,3] = 0;
            Matrix4x4 wRtr = cThighR.fromWorld;
            wRtr[0,3] = 0;
            wRtr[1,3] = 0;
            wRtr[2,3] = 0;
            Matrix4x4 hrRtr = hrRw * wRtr;
            knee_r.localRotation = hrRtr.rotation;
            
            // shin rotation left
            Matrix4x4 tlRw = cThighL.toWorld;
            tlRw[0,3] = 0;
            tlRw[1,3] = 0;
            tlRw[2,3] = 0;
            Matrix4x4 wRshl = cShinL.fromWorld;
            wRshl[0,3] = 0;
            wRshl[1,3] = 0;
            wRshl[2,3] = 0;
            Matrix4x4 tlRshl = tlRw * wRshl;
            foot_l.localRotation = tlRshl.rotation;
            
            // shin rotation left
            Matrix4x4 trRw = cThighL.toWorld;
            trRw[0,3] = 0;
            trRw[1,3] = 0;
            trRw[2,3] = 0;
            Matrix4x4 wRshr = cShinL.fromWorld;
            wRshr[0,3] = 0;
            wRshr[1,3] = 0;
            wRshr[2,3] = 0;
            Matrix4x4 trRshr = trRw * wRshr;
            foot_r.localRotation = trRshr.rotation;
            
            _modelMatched = true;
        }
	}
    
    private void paintCoordinateSystem(ref GameObject coordinateSystem,
                                      CoordinateSystem c)
    {
        GameObject modelXAxis = new GameObject();
        GameObject modelYAxis = new GameObject();
        GameObject modelZAxis = new GameObject();
            
        modelXAxis.transform.parent = coordinateSystem.transform;
        modelYAxis.transform.parent = coordinateSystem.transform;
        modelZAxis.transform.parent = coordinateSystem.transform;
        Vector3[] positions = new Vector3[2];
        positions[0] = c.o;
            
        modelXAxis.AddComponent<LineRenderer>();
        positions[1] = c.o + (c.x * 100.0f);
        LineRenderer xAxisRenderer = modelXAxis.GetComponent<LineRenderer>();
        xAxisRenderer.SetPositions(positions);
        xAxisRenderer.material.color = new Color(1.0f, 0.0f, 0.0f);
            
        modelYAxis.AddComponent<LineRenderer>();
        positions[1] = c.o + (c.y * 100.0f);
        LineRenderer yAxisRenderer = modelYAxis.GetComponent<LineRenderer>();
        yAxisRenderer.SetPositions(positions);
        yAxisRenderer.material.color = new Color(0.0f, 1.0f, 0.0f);
            
        modelZAxis.AddComponent<LineRenderer>();
        positions[1] = c.o + (c.z * 100.0f);
        LineRenderer zAxisRenderer = modelZAxis.GetComponent<LineRenderer>();
        zAxisRenderer.SetPositions(positions);
        zAxisRenderer.material.color = new Color(0.0f, 0.0f, 1.0f);
    }
    
    private CoordinateSystem initializeCoordinateSystem(Vector3 o,
                                                        Vector3 x,
                                                        Vector3 y,
                                                        Vector3 z)
    {
        CoordinateSystem result = new CoordinateSystem();
        
        result.o = o;
        result.x = x;
        result.y = y;
        result.z = z;
        
        Matrix4x4 wRm = Matrix4x4.identity;
        wRm[0,0] = Vector3.Dot(Vector3.right, result.x);
        wRm[1,0] = Vector3.Dot(Vector3.right, result.y);
        wRm[2,0] = Vector3.Dot(Vector3.right, result.z);
        wRm[0,1] = Vector3.Dot(Vector3.up, result.x);
        wRm[1,1] = Vector3.Dot(Vector3.up, result.y);
        wRm[2,1] = Vector3.Dot(Vector3.up, result.z);
        wRm[0,2] = Vector3.Dot(Vector3.forward, result.x);
        wRm[1,2] = Vector3.Dot(Vector3.forward, result.y);
        wRm[2,2] = Vector3.Dot(Vector3.forward, result.z);
        result.toWorld = wRm * Matrix4x4.Translate(Vector3.zero - result.o);
        
        Matrix4x4 mRw = wRm.transpose;
        result.fromWorld = mRw * Matrix4x4.Translate(result.o);
        
        return result;
    }
    
}
