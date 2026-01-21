using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Numerics;
using Raylib_cs;
using System.ComponentModel;

namespace Doomlike
{
    internal class Player
    {
        public Vector2 position;
        public float verticalPosition;
        
        public Vector2 direction;
        public float rotationRad;

        public float walkSpeed;
        public float strafeSpeed;
        public float turnSpeedDeg;
        public float fovDeg;

        public Player()
        {
            direction = Vector2.UnitY;
            rotationRad = 0.0f;
            walkSpeed = 150;
            strafeSpeed = 130;
            turnSpeedDeg = 120;
            fovDeg = 90;
        }

        public void Update(float scale)
        {
            float dt = Raylib.GetFrameTime();

            Matrix3x2 rotmat = Matrix3x2.CreateRotation(rotationRad);
            direction = Vector2.Transform(Vector2.UnitY, rotmat);

            Matrix3x2 ninety = Matrix3x2.CreateRotation(90 * Raylib.DEG2RAD);
            Vector2 right = Vector2.Transform(direction, ninety);

            float ws = walkSpeed * scale;
            float ss = strafeSpeed * scale;

            if (Raylib.IsKeyDown(KeyboardKey.W))
            {
                position += direction * ws * dt;
            }
            if (Raylib.IsKeyDown(KeyboardKey.A))
            {
                position -= right * ss * dt;
            }
            if (Raylib.IsKeyDown(KeyboardKey.S))
            {
                position -= direction * ws * dt; 
            }
            if (Raylib.IsKeyDown(KeyboardKey.D))
            {
                position += right * ss * dt;

            }


            if (Raylib.IsKeyDown(KeyboardKey.Q))
            {
                rotationRad -= turnSpeedDeg * Raylib.DEG2RAD * dt;
            }
            if (Raylib.IsKeyDown(KeyboardKey.E))
            {
                rotationRad += turnSpeedDeg * Raylib.DEG2RAD * dt;
            }
            
        }

        public void DrawTopdownRotated(Vector2 worldPosition, float rotationRadians)
        {
            Raylib.DrawCircleLinesV(worldPosition, 5, Color.White);

            // The positive Y axis is depth
            {
                float forwardAngle = rotationRadians;
                Matrix3x2 rotmat = Matrix3x2.CreateRotation(forwardAngle);
                Vector2 drawDirection = Vector2.Transform(Vector2.UnitY, rotmat);
                Raylib.DrawLineV(worldPosition, worldPosition + drawDirection * 10, Color.White);
            }

            {
                // Draw Fov pie
                float fovAngle = rotationRadians;
                Matrix3x2 rotmatFov = Matrix3x2.CreateRotation(fovAngle);
                Vector2 fovDirection = Vector2.Transform(Vector2.UnitY, rotmatFov);
                Matrix3x2 rotmatFovL = Matrix3x2.CreateRotation(-fovDeg / 2 * Raylib.DEG2RAD);
                Matrix3x2 rotmatFovR = Matrix3x2.CreateRotation(+fovDeg / 2 * Raylib.DEG2RAD);
                Vector2 leftSide = Vector2.Transform(fovDirection, rotmatFovL);
                Vector2 rightSide = Vector2.Transform(fovDirection, rotmatFovR);
                Raylib.DrawLineV(worldPosition, worldPosition + leftSide * 150, Color.LightGray);
                Raylib.DrawLineV(worldPosition, worldPosition + rightSide * 150, Color.LightGray);
            }
        }

        public void DrawTopdown()
        {
            DrawTopdownRotated(position, rotationRad);
        }
    }
}
