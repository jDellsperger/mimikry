using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class SpotterTextureBehaviour : MonoBehaviour {
    
    public byte spotterId;
    
    private Texture2D _frameTexture;
    private MessageQueueHandler.Frame _frame;
    private RawImage _rawFrame;
    private MessageQueueHandler _messageQueueHandler;
    bool _initialized = false;
    
	void Start () 
    {
        _messageQueueHandler = MessageQueueHandler.getInstance();
        _rawFrame = gameObject.GetComponent<RawImage>();
	}
	
	void Update () 
    {
        if (_messageQueueHandler.getDebugFramesIfChanged(spotterId, ref _frame))
        {
            if (!_initialized)
            {
                _frameTexture = new Texture2D(_frame.width, _frame.height);
                _frameTexture.wrapMode = TextureWrapMode.Clamp;
                _frame = new MessageQueueHandler.Frame();
                _frame.memory = new byte[_frame.pitch * _frame.height];
                
                _initialized = true;
            }
            
            /*
                        for (int y = 0; 
                             y < _frame.height;
                             y++)
                        {
                            for (int x = 0;
                                 x < _frame.width;
                                 x++)
                            {
                                // NOTE(jan): this only works for frames with 1 byte per pixel
                                float color = _frame.memory[y * _frame.width + x] / 255.0f;
                                _frameTexture.SetPixel(x, (_frame.height - 1)  - y, 
                                                       new Color(color, color, color));
                            }
                        }
                        
                        _frameTexture.Apply();
                                    */
            
            ImageConversion.LoadImage(_frameTexture, _frame.memory);
            
            _rawFrame.texture = _frameTexture;
        }
	}
}
