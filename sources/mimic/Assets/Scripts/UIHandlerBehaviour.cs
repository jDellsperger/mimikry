using UnityEngine;
using UnityEngine.UI;

public class UIHandlerBehaviour : MonoBehaviour {
    
    public UnityEngine.UI.Slider binarizationThresholdSlider;
    
    private MessageQueueHandler _messageQueueHandler;
    private GameObject[] monitors;
    private GameObject rays;
    private GameObject intersections;
    private GameObject rayToggle;
    private GameObject intersectionToggle;
    private Visualizer _visualizer;
    private GameObject _debugVisualizer;

    void Start() {
        _debugVisualizer = GameObject.Find("DebugVisualizer");
        _visualizer = (Visualizer) _debugVisualizer.GetComponent(typeof(Visualizer));
        monitors = GameObject.FindGameObjectsWithTag ("Monitor");
        rays = GameObject.Find("Ray Container"); 
        intersections = GameObject.Find("Intersection Container");
        rayToggle = GameObject.Find("RayVisualizerToggle");
        intersectionToggle = GameObject.Find("IntersectionVisualizerToggle");
    }
    
    public UIHandlerBehaviour()
    {
        _messageQueueHandler = MessageQueueHandler.getInstance();
    }
    
    public void SendEstimatePoseCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_EstimatePose;
        _messageQueueHandler.SendCommand(commandType);
    }
    
    public void SendSendGrayscaleCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_SendGrayscale;
        _messageQueueHandler.SendCommand(commandType);
        
        foreach(GameObject go in monitors)
        {
            go.SetActive (true);
        }
    }
    
    public void NoFramesCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_NoFrames;
        _messageQueueHandler.SendCommand(commandType);
    
        foreach(GameObject go in monitors)
        {
            go.SetActive (false);
        }
    }

    public void SendSendBinarizedCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_SendBinarized;
        _messageQueueHandler.SendCommand(commandType);
    
        foreach(GameObject go in monitors)
        {
            go.SetActive (true);
        }
    }
    
    public void SendStartRecordingRaysCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_SaveRaysToFile;
        _messageQueueHandler.SendCommand(commandType);
    }
    
    public void SendStartRecordingFramesCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_SaveFramesToFile;
        _messageQueueHandler.SendCommand(commandType);
    }
    
    public void SendMatchModelCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_MatchModel;
        _messageQueueHandler.SendCommand(commandType);
    }

    public void SendStopSystemCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_StopSystem;
        _messageQueueHandler.SendCommand(commandType);
    }
    
    public void SendStartDebuggingCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_StartDebugging;
        _messageQueueHandler.SendCommand(commandType);
        rayToggle.GetComponent<Toggle>().isOn = true;
        intersectionToggle.GetComponent<Toggle>().isOn = true;
        rayToggle.SetActive(true);
        intersectionToggle.SetActive(true);
    }
    
    public void SendStopDebuggingCommand()
    {
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.commandType_StopDebugging;
        _messageQueueHandler.SendCommand(commandType);
        rayToggle.GetComponent<Toggle>().isOn = false;
        intersectionToggle.GetComponent<Toggle>().isOn = false;
        rayToggle.SetActive(false);
        intersectionToggle.SetActive(false);
    }

    public void VisualizeRaysCommand()
    {
        if(rayToggle.GetComponent<Toggle>().isOn == true) {
            _visualizer.raysActive = true;
        }
        else {
            _visualizer.raysActive = false;
        }
    }

    public void VisualizeIntersectionsCommand()
    {   
        if(intersectionToggle.GetComponent<Toggle>().isOn == true) {
            _visualizer.intersectionsActive = true;
        }
        else {
            _visualizer.intersectionsActive = false;
        }
    }
    
    public void SendBinarizationThresholdCommand()
    {
        byte[] data = new byte[1];
        data[0] = (byte)binarizationThresholdSlider.value;
        
        Debug.Log("Binarization threshold: " + data[0]);
        
        MessageQueueHandler.CommandType commandType = MessageQueueHandler.CommandType.CommandType_BinarizationThreshold;
        _messageQueueHandler.SendCommand(commandType, data, 1);
    }
    
}

