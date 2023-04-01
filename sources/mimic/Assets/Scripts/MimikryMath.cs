using UnityEngine;

public struct V2
{
    public float x, y;
};

public struct V3
{
    public float x, y, z;
};

public unsafe struct M4x4
{
    public fixed float e[16];
};

public struct Ray
{
    public V3 origin;
    public V3 direction;
};

class MimikryMath
{
    public static float valM4x4(M4x4 m, int col, int row)
    {
        float result;
        
        unsafe
        {
            int index = col*4 + row;
            result = m.e[index];
        }
        
        return result;
    }
    
    public static void printM4x4(M4x4 m)
    {
        for (int row = 0; row < 4; row++)
        {
            Debug.Log(valM4x4(m, 0, row) + ", " +
                      valM4x4(m, 1, row) + ", " +
                      valM4x4(m, 2, row) + ", " +
                      valM4x4(m, 3, row));
        }
    }
    
    public void printRay(Ray ray)
    {
        Debug.Log("Ray - origin: " + 
                  ray.origin.x + " / " + ray.origin.y + " / " + ray.origin.z +
                  " - dir: " + 
                  ray.direction.x + " / " + ray.direction.y + " / " + ray.direction.z);
    }
    
    public static Vector3 V3toVector3(V3 v)
    {
        Vector3 result = new Vector3();
        
        result.x = v.x;
        result.y = -v.z;
        result.z = v.y;
        
        return result;
    }
}

