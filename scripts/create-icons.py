"""Deterministic app icons derived from Arcfield's existing curved-tile motif."""
from pathlib import Path
from PIL import Image, ImageDraw
import math
root=Path(__file__).resolve().parents[1]/'dist'/'icons'
root.mkdir(exist_ok=True)
def icon(size,maskable=False):
 scale=4;s=size*scale
 im=Image.new('RGB',(s,s),'#101714');d=ImageDraw.Draw(im)
 # Keep the entire recognizable mark inside the maskable central safe circle.
 unit=s*(.22 if maskable else .27);left=s/2-unit;top=s/2-unit
 colors=['#d8ec9d','#8fbfae','#d8ec9d','#8fbfae']
 for row in range(2):
  for col in range(2):
   x=left+col*unit;y=top+row*unit
   d.rounded_rectangle((x+unit*.05,y+unit*.05,x+unit*.95,y+unit*.95),radius=unit*.09,fill='#1e2a20')
   # Two quarter-circle pairings meeting the tile's edge midpoints.
   corners=[(x+unit,y,math.pi/2,math.pi),(x,y+unit,-math.pi/2,0)] if (row+col)%2==0 else [(x,y,0,math.pi/2),(x+unit,y+unit,math.pi,math.pi*1.5)]
   for cx,cy,a,b in corners:
    points=[(cx+unit*.5*math.cos(a+(b-a)*k/40),cy+unit*.5*math.sin(a+(b-a)*k/40)) for k in range(41)]
    d.line(points,fill=colors[row*2+col],width=round(unit*.085),joint='curve')
 # Small marble, kept well inside maskable padding.
 cx=left+unit*.5;cy=top+unit*.12;r=unit*.075
 d.ellipse((cx-r,cy-r,cx+r,cy+r),fill='#fff4ce')
 return im.resize((size,size),Image.Resampling.LANCZOS)
for n in [192,512]:icon(n).save(root/f'icon-{n}.png')
icon(512,True).save(root/'maskable-512.png')
icon(180).save(root/'apple-touch-icon.png')
icon(32).save(root/'favicon-32.png')
