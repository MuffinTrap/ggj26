using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace Doomlike
{
    internal class Map
    {
        public List<Sector> sectors;

        public Map()
        {
            sectors = new List<Sector>();
        }

        public void DrawTopdown()
        {
            foreach(Sector s in sectors)
            {
                s.DrawTopdown();
            }
        }
        public void DrawTopdownRotated(Player player)
        {
            foreach (Sector s in sectors)
            {
                s.DrawTopdownRotated(player);
            }
        }

        public void DrawFirstPerson(Player player)
        {
            foreach (Sector s in sectors)
            {
                s.DrawFirstPerson(player);
            }
        }
        public void DrawModels()
        {
            foreach (Sector s in sectors)
            {
                s.DrawModels(Vector3.Zero);
            }
        }
    }
}
