using System.Diagnostics;
using UnityEngine;
using System.Threading;
using NetMQ;
using NetMQ.Sockets;
using System.Runtime.InteropServices;
using System;
using System.Collections.Generic;

public class MessageQueueHandler
{
    private static MessageQueueHandler _instance;
    private readonly Thread _listenerWorker;
    private bool _listenerCancelled;
    private const long ContactThreshold = 1000;
    
    private PushSocket _pushSocket = new PushSocket();
    
    private object _modelPointsLock = new object();
    private Dictionary<int, V3> _modelPoints;
    private bool _modelPointsUpdatedSinceGet = false; 
    
    private object _rayLock = new object();
    private List<List<Ray>> _rays;
    private int _rayFrontBufferIndex;
    private int _rayBackBufferIndex;
    private const int RAY_BUFFER_COUNT = 2;
    private bool _raysChangedSinceGet = false;
    
    private object _cameraLock = new object();
    private Dictionary<byte, Matrix4x4> _cameras;
    
    private object _frameLock = new object();
    private Dictionary<byte, Frame> _debugFrames;
    private int _debugFramesFrontBufferIndex;
    private int _debugFramesBackBufferIndex;
    private const int DEBUG_FRAMES_BUFFER_COUNT = 2;
    
    private object _intersectionLock = new object();
    private List<V3> _intersections = new List<V3>();
    private bool _intersectionsChangedSinceGet = false;
    
    public enum MessageType
    {
        MessageType_None,
        MessageType_HelloReq,
        MessageType_HelloRep,
        MessageType_Payload,
        MessageType_Command,
        
        // Debug message types
        MessageType_DebugCameraPose,
        MessageType_DebugRays,
        MessageType_DebugFrame,
        MessageType_DebugIntersections
    };
    
    public enum CommandType : uint
    {
        CommandType_None,
        CommandType_GrabFrame,
        CommandType_EstimatePose,
        CommandType_SendGrayscale,
        CommandType_SendBinarized,
        CommandType_SaveRaysToFile,
        CommandType_SaveFramesToFile,
        CommandType_MatchModel,
        CommandType_BinarizationThreshold,
        CommandType_NoFrames,
        CommandType_StopSystem,
        CommandType_StartDebugging,
        commandType_StopDebugging
    };
    
    public struct CommandHeader
    {
        public CommandType commandType;
    };
    
    public struct MessageHeader
    {
        public MessageType type;
        public byte spotterID;
        public uint payloadSize; //Message size in byte
    };
    
    public struct DebugFrameInfo
    {
        public int width;
        public int height;
        public int bytesPerPixel;
    };
    
    public struct Frame
    {
        public byte[] memory;
        public int width, height;
        public int bytesPerPixel;
        public int pitch;
    };
    
    // NOTE(jan): code stolen from https://stackoverflow.com/questions/2871/reading-a-c-c-data-structure-in-c-sharp-from-a-byte-array
    unsafe T ByteArrayToStructure<T>(byte[] bytes, int index = 0) where T : struct
    {
        fixed (byte* ptr = &bytes[index])
        {
            return (T)Marshal.PtrToStructure((IntPtr)ptr, typeof(T));
        }
    }
    
    // NOTE(jan): code stolen from https://stackoverflow.com/questions/3278827/how-to-convert-a-structure-to-a-byte-array-in-c
    private void CopyStructureToByteArray<T>(ref byte[] arr, 
                                             int offset,
                                             T structure) where T : struct
    {
        int size = Marshal.SizeOf(structure);
        
        IntPtr ptr = Marshal.AllocHGlobal(size);
        Marshal.StructureToPtr(structure, ptr, true);
        Marshal.Copy(ptr, arr, offset, size);
        Marshal.FreeHGlobal(ptr);
    }
    
    public bool getModelPointsIfChanged(ref Dictionary<int, V3> modelPoints)
    {
        bool result = false;
        
        lock (_modelPointsLock)
        {
            if (_modelPointsUpdatedSinceGet)
            {
                _modelPointsUpdatedSinceGet = false;
                modelPoints = _modelPoints;
                result = true;
            }
        }
        
        return result;
    }
    
    public bool getRaysIfChanged(ref List<Ray> rays)
    {
        bool result = false;
        
        lock (_rayLock)
        {
            if (_raysChangedSinceGet)
            {
                _raysChangedSinceGet = false;
                rays = _rays[_rayFrontBufferIndex];
                result = true;
            }
        }
        
        return result;
    }
    
    public bool getIntersectionsIfChanged(ref List<V3> intersections)
    {
        bool result = false;
        
        lock (_intersectionLock)
        {
            if (_intersectionsChangedSinceGet)
            {
                _intersectionsChangedSinceGet = false;
                intersections = _intersections;
                result = true;
            }
        }
        
        return result;
    }
    
    public bool getDebugFramesIfChanged(byte spotterId, ref Frame frame)
    {
        bool result = false;
        
        lock (_frameLock)
        {
            if (_debugFrames.ContainsKey(spotterId))
            {
                frame = _debugFrames[spotterId];
                _debugFrames.Remove(spotterId);
                result = true;
            }
        }
        
        return result;
    }
    
    public Dictionary<byte, Matrix4x4> getCameras()
    {
        lock (_cameraLock)
        {
            Dictionary<byte, Matrix4x4> result =
                new Dictionary<byte, Matrix4x4>();
            
            foreach (var cameraAndIndex in _cameras)
            {
                result.Add(cameraAndIndex.Key, cameraAndIndex.Value);
            }
            
            _cameras.Clear();
            
            return result;
        }
    }
    
    private void ListenerWork()
    {
        using (var socket = new PullSocket())
        {
            socket.Bind("tcp://*:5556");
            
            while (!_listenerCancelled)
            {
                byte[] message;
                if (socket.TryReceiveFrameBytes(out message))
                {
                    MessageHeader header =
                        ByteArrayToStructure<MessageHeader>(message);
                    UnityEngine.Debug.Log("Received message with type " + header.type);
                    
                    switch (header.type)
                    {
                        case MessageType.MessageType_Payload: {
                            // TODO(jan): actual model
                            Dictionary<int, V3> modelPoints = new Dictionary<int, V3>();
                            
                            int payloadSizeHandled = 0;
                            int i = 0;
                            while (payloadSizeHandled < header.payloadSize)
                            {
                                byte[] payload =
                                    new byte[header.payloadSize - payloadSizeHandled];
                                Array.Copy(message,
                                           Marshal.SizeOf(header) + payloadSizeHandled,
                                           payload,
                                           0,
                                           header.payloadSize - payloadSizeHandled);
                                V3 point = ByteArrayToStructure<V3>(payload);
                                modelPoints.Add(i, point);
                                
                                payloadSizeHandled += Marshal.SizeOf(point);
                                i++;
                            }
                            
                            lock (_modelPointsLock)
                            {
                                _modelPoints = modelPoints;
                                _modelPointsUpdatedSinceGet = true;
                            }
                        } break;
                        
                        case MessageType.MessageType_DebugCameraPose: {
                            byte[] payload = new byte[header.payloadSize];
                            Array.Copy(message,
                                       Marshal.SizeOf(header),
                                       payload,
                                       0,
                                       header.payloadSize);
                            byte cameraId = header.spotterID;
                            
                            Array.Copy(message,
                                       Marshal.SizeOf(header),
                                       payload,
                                       0,
                                       header.payloadSize);
                            M4x4 wTc = ByteArrayToStructure<M4x4>(payload);
                            
                            Matrix4x4 camera = new Matrix4x4();
                            for (int col = 0; col < 4; col++)
                            {
                                Vector4 v = new Vector4();
                                for (int row = 0; row < 4; row++)
                                {
                                    v[row] = MimikryMath.valM4x4(wTc, col, row);
                                }
                                
                                camera.SetColumn(col, v);
                            }
                            
                            lock (_cameraLock)
                            {
                                if (_cameras.ContainsKey(cameraId))
                                {
                                    _cameras.Remove(cameraId);
                                }
                                
                                _cameras.Add(cameraId, camera);
                            }
                        } break;
                        
                        case MessageType.MessageType_DebugRays: {
                            int payloadSizeHandled = 0;
                            _rays[_rayBackBufferIndex] = new List<Ray>();
                            int rayCount = 0;
                            while (payloadSizeHandled < header.payloadSize)
                            {
                                byte[] payload =
                                    new byte[header.payloadSize - payloadSizeHandled];
                                Array.Copy(message,
                                           Marshal.SizeOf(header) + payloadSizeHandled,
                                           payload,
                                           0,
                                           header.payloadSize - payloadSizeHandled);
                                Ray ray = ByteArrayToStructure<Ray>(payload);
                                _rays[_rayBackBufferIndex].Add(ray);
                                
                                rayCount++;
                                payloadSizeHandled += Marshal.SizeOf(ray);
                            }
                            
                            lock (_rayLock) 
                            {
                                _rayFrontBufferIndex = _rayBackBufferIndex;
                                _rayBackBufferIndex =
                                    (_rayBackBufferIndex + 1) % RAY_BUFFER_COUNT;
                                _raysChangedSinceGet = true;
                            }
                        } break;
                        
                        case MessageType.MessageType_DebugFrame: {
                            byte[] payload = new byte[header.payloadSize];
                            Array.Copy(message,
                                       Marshal.SizeOf(header),
                                       payload,
                                       0,
                                       header.payloadSize);
                            DebugFrameInfo info =
                                ByteArrayToStructure<DebugFrameInfo>(payload, 0);
                            Frame frame = new Frame();
                            frame.memory =
                                new byte[header.payloadSize];
                            Array.Copy(message,
                                       Marshal.SizeOf(header) + 
                                       Marshal.SizeOf(info),
                                       frame.memory,
                                       0,
                                       header.payloadSize - Marshal.SizeOf(info));
                            frame.width = info.width;
                            frame.height = info.height;
                            frame.bytesPerPixel = info.bytesPerPixel;
                            frame.pitch = info.width * info.bytesPerPixel;
                            
                            lock(_frameLock)
                            {
                                if (_debugFrames.ContainsKey(header.spotterID))
                                {
                                    _debugFrames.Remove(header.spotterID);
                                }
                                _debugFrames.Add(header.spotterID, frame);
                            }
                        } break;
                        
                        case MessageType.MessageType_DebugIntersections: {
                            int payloadSizeHandled = 0;
                            List<V3> intersections = new List<V3>();
                            
                            int intersectionCount = 0;
                            
                            while (payloadSizeHandled < header.payloadSize)
                            {
                                byte[] payload =
                                    new byte[header.payloadSize - payloadSizeHandled];
                                Array.Copy(message,
                                           Marshal.SizeOf(header) + payloadSizeHandled,
                                           payload,
                                           0,
                                           header.payloadSize - payloadSizeHandled);
                                V3 intersection = ByteArrayToStructure<V3>(payload);
                                intersections.Add(intersection);
                                payloadSizeHandled += Marshal.SizeOf(intersection);
                                
                                intersectionCount++;
                            }
                            
                            UnityEngine.Debug.Log("Received " + intersectionCount + " intersections");
                            
                            lock (_intersectionLock)
                            {
                                _intersections = intersections;
                                _intersectionsChangedSinceGet = true;
                            }
                        } break;
                    }
                }
            }
            
            socket.Dispose();
        }
    }
    
    public void SendCommand(CommandType commandType, byte[] data, uint dataSize)
    {
        MessageHeader header = new MessageHeader();
        CommandHeader commandHeader = new CommandHeader();
        
        header.type = MessageType.MessageType_Command;
        header.payloadSize = (uint)Marshal.SizeOf(commandHeader) + dataSize;
        commandHeader.commandType = commandType;
        
        byte[] payload =
            new byte[Marshal.SizeOf(header) + header.payloadSize];
        CopyStructureToByteArray<MessageHeader>(ref payload,
                                                0,
                                                header);
        CopyStructureToByteArray(ref payload,
                                 Marshal.SizeOf(header),
                                 commandHeader);
        
        if (dataSize > 0)
        {
            Array.Copy(data,
                       0,
                       payload,
                       Marshal.SizeOf(header) + Marshal.SizeOf(commandHeader),
                       dataSize);
        }
        
        _pushSocket.SendFrame(payload);
    }
    
    public void SendCommand(CommandType commandType)
    {
        byte[] data = new byte[0];
        SendCommand(commandType, data, 0);
    }
    
    private MessageQueueHandler()
    {
        _listenerWorker = new Thread(ListenerWork);
        
        // NOTE(jan): Creating ray buffers
        _rayBackBufferIndex = 0;
        _rayFrontBufferIndex = 0;
        
        _rays = new List<List<Ray>>();
        _rays.Add(new List<Ray>());
        _rays.Add(new List<Ray>());
        
        _cameras = new Dictionary<byte, Matrix4x4>();
        _debugFrames = new Dictionary<byte, Frame>();
    }
    
    public void Start()
    {
        _pushSocket.Connect("tcp://localhost:5557");
        
        _listenerCancelled = false;
        _listenerWorker.Start();
    }
    
    public void Stop()
    {
        _listenerCancelled = true;
        _listenerWorker.Join();
        
        _pushSocket.Dispose();
        NetMQConfig.Cleanup();
    }
    
    public static MessageQueueHandler getInstance()
    {
        if (_instance == null)
        {
            _instance = new MessageQueueHandler();
            _instance.Start();
        }
        
        return _instance;
    }
}
