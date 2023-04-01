using UnityEngine;
using System.Collections.Generic;

public class Visualizer : MonoBehaviour 
{
    private Vector3 intersectionsCOM;
    public int chessboardRowCount = 6;
    public int chessboardColCount = 9;
    public float chessboardFieldInCm = 12.0f;
    
    public int modelPointCount = 13;
    public int trajectoryCount = 10;
    public bool raysActive = true;
    public bool intersectionsActive = true;
    
    private MessageQueueHandler _messageQueueHandler;
    private Dictionary<int, V3> _modelPoints;
    private Dictionary<int, List<GameObject>> _modelPointTrajectories;
    private int _modelPointTrajectoryIndex;
    private GameObject _rayContainer;
    private GameObject _intersectionContainer;
    private Dictionary<byte, GameObject> _cameras;
    
    private void Start()
    {
        _modelPointTrajectoryIndex = 0;
        _modelPointTrajectories = new Dictionary<int, List<GameObject>>();
        
        for (int i = 0; i < modelPointCount; i++)
        {
            _modelPointTrajectories.Add(i, new List<GameObject>());
        }
        
        intersectionsCOM = new Vector3();
        _rayContainer = new GameObject();
        _rayContainer.name = "Ray Container";
        
        _intersectionContainer = new GameObject();
        _intersectionContainer.name = "Intersection Container";
        
        _messageQueueHandler = MessageQueueHandler.getInstance();
        
        _cameras = new Dictionary<byte, GameObject>();
        
        //CreateChessboard();
    }
    
    private void CreateChessboard()
    {
        GameObject chessboard = new GameObject();
        chessboard.name = "Chessboard";
        float startX = -1.0f * (chessboardFieldInCm / 2.0f);
        float startY = -1.0f * (chessboardFieldInCm / 2.0f);
        float curX = startX;
        float curY = startY;
        bool black = true;
        for (int row = 0;
             row < chessboardRowCount;
             row++)
        {
            for (int col = 0;
                 col < chessboardColCount;
                 col++)
            {
                GameObject cubeObj = GameObject.CreatePrimitive(PrimitiveType.Cube);
                cubeObj.transform.parent = chessboard.transform;
                cubeObj.transform.localScale = new Vector3(chessboardFieldInCm,
                                                           chessboardFieldInCm,
                                                           0.000001f);
                cubeObj.transform.position = 
                    new Vector3(curX, curY, 0.0f);
                
                MeshRenderer cubeRenderer = cubeObj.GetComponent<MeshRenderer>();
                
                if (black)
                {
                    cubeRenderer.material.color = new Color(0.0f, 0.0f, 0.0f);
                    black = false;
                }
                else
                {
                    cubeRenderer.material.color = new Color(1.0f, 1.0f, 1.0f);
                    black = true;
                }
                curX += chessboardFieldInCm;
            }
            
            curX = startX;
            curY += chessboardFieldInCm;
        }
    }
    
    private void Update()
    {
        Dictionary<byte, Matrix4x4> cameras = _messageQueueHandler.getCameras();
        foreach (var cameraAndIndex in cameras)
        {
            byte index = cameraAndIndex.Key;
            Matrix4x4 cameraPose = cameraAndIndex.Value;
            
            if (_cameras.ContainsKey(index))
            {
                Destroy(_cameras[index]);
                _cameras.Remove(index);
            }
            
            GameObject camObj = GameObject.CreatePrimitive(PrimitiveType.Cube);
            Quaternion rQ = cameraPose.rotation;
            rQ = Quaternion.Inverse(rQ);
            // TODO(jan): this doesn't seem right, think about it again
            Vector3 rE = rQ.eulerAngles;
            rE.x = rE.x;
            rE.y = -rE.z;
            rE.z = rE.y;
            Vector4 t = cameraPose.GetColumn(3);
            float tmpZ = t.z;
            t.z = t.y;
            t.y = -tmpZ;
            camObj.transform.Rotate(rE);
            camObj.transform.position = t;
            //camObj.AddComponent<Camera>();
            
            _cameras.Add(index, camObj);
        }
        
        List<Ray> rays = new List<Ray>();
        if (_messageQueueHandler.getRaysIfChanged(ref rays))
        {
            // TODO(jan): destroy all rays. Is there a better way to do this?
            Destroy(_rayContainer);
            
            if(raysActive) {
                _rayContainer = new GameObject();
                _rayContainer.name = "Ray Container";
                
                int rayIndex = 0;
                foreach (Ray ray in rays)
                {
                    GameObject rayObj = new GameObject();
                    rayObj.name = "Ray " + rayIndex;
                    rayIndex++;
                    rayObj.AddComponent<LineRenderer>();
                    LineRenderer rayRenderer = rayObj.GetComponent<LineRenderer>();
                    Vector3 origin =
                        new Vector3(ray.origin.x, -ray.origin.z, ray.origin.y);
                    Vector3 direction =
                        new Vector3(ray.direction.x, -ray.direction.z, ray.direction.y);
                    Vector3[] positions = new Vector3[2];
                    positions[0] = origin;
                    positions[1] = origin + (10000.0f * direction);
                    rayRenderer.SetPositions(positions);
                    rayRenderer.material.color = new Color(1.0f, 0.0f, 0.0f);
                    rayRenderer.startWidth = 0.1f;
                    rayRenderer.endWidth = 0.1f;
                    rayObj.transform.parent = _rayContainer.transform;
                }
            }
        }
        
        List<V3> intersections = new List<V3>();
        if (_messageQueueHandler.getIntersectionsIfChanged(ref intersections))
        {
            Destroy(_intersectionContainer);
            
            if(intersectionsActive) {
                _intersectionContainer = new GameObject();
                _intersectionContainer.name = "Intersection Container";
                
                float x = 0;
                float y = 0;
                float z = 0;
                int count = 0;
                
                foreach (V3 intersection in intersections)
                {
                    GameObject isectObj =
                        GameObject.CreatePrimitive(PrimitiveType.Sphere);
                    
                    MeshRenderer isectRenderer = isectObj.GetComponent<MeshRenderer>();
                    isectRenderer.material.color = new Color(0.0f, 0.0f, 1.0f);
                    
                    isectObj.transform.position = 
                        new Vector3(intersection.x, -intersection.z, intersection.y);
                    
                    x += intersection.x;
                    y -= intersection.z;
                    z += intersection.y;
                    count++;
                    
                    isectObj.transform.parent = _intersectionContainer.transform;
                }
                
                intersectionsCOM = new Vector3(x/count, y/count, z/count);
            }
        }
    }
    
    public Transform getIntersectionsCOM()
    {
        gameObject.transform.position = intersectionsCOM;
        return gameObject.transform;
    }
    
    private void OnDestroy()
    {
        _messageQueueHandler.Stop();
    }
}
