/* Arcfield compact deterministic simulation. No allocation, filesystem, or clocks.
 * C99; compile natively or to freestanding wasm32. All time is simulation time.
 * Limits match the UI: 24x18 tiles, 120s, 30 injections/s, 10 tiles/s.
 */
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __SIZE_TYPE__ size_t;
extern double cos(double);
extern double sin(double);
#define MAX_TILES 432
#define MAX_PARTICLES 3601
#define MAX_EVENTS 3600
#define MAX_SECONDS 120
#define DT (1.0/60.0)
typedef struct {int x,y,entry,exit;double u,px,py;int dead,positioned;} Particle;
typedef struct {double time;int x,y,entry;} Event;
static uint8_t tiles[MAX_TILES];
static int bins[MAX_SECONDS];
static Event events[MAX_EVENTS];
static Particle particles[MAX_PARTICLES];
static double schedule_input[MAX_EVENTS*4],particle_output[MAX_PARTICLES*7];
static int cols,rows,duration,event_count,event_at,particle_count,collisions,exits,steps;
static double speed,sim_time;
static uint32_t random_state;
static const int exits_table[3][4]={{1,0,3,2},{3,2,1,0},{2,3,0,1}};
static const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
static double random_value(void){random_state+=0x6D2B79F5u;uint32_t t=(random_state^(random_state>>15))*(random_state|1u);t^=t+((t^(t>>7))*(t|61u));return (double)(t^(t>>14))/4294967296.0;}
static void locate(Particle*p){if(p->positioned)return;static const double ex[4]={.5,1,.5,0},ey[4]={0,.5,1,.5};double ax=ex[p->entry],ay=ey[p->entry],bx=ex[p->exit],by=ey[p->exit];if((p->entry+2)%4==p->exit){p->px=p->x+ax+(bx-ax)*p->u;p->py=p->y+ay+(by-ay)*p->u;}else{int cx=p->entry==1||p->exit==1,cy=p->entry==2||p->exit==2;/* Quarter-turn interpolation avoids atan2 in the hot path. */double vx=ax-cx,vy=ay-cy,cross=vx*(by-cy)-vy*(bx-cx),a=(cross>0?1:-1)*1.5707963267948966*p->u,co=cos(a),si=sin(a);p->px=p->x+cx+vx*co-vy*si;p->py=p->y+cy+vx*si+vy*co;}p->positioned=1;}
uintptr_t tiles_ptr(void){return(uintptr_t)tiles;}
uintptr_t schedule_ptr(void){return(uintptr_t)schedule_input;}
uintptr_t bins_ptr(void){return(uintptr_t)bins;}
uintptr_t particles_ptr(void){return(uintptr_t)particle_output;}
int configure(int w,int h,int seconds,double velocity,int count,uint32_t seed){if(w<1||h<1||w>24||h>18||w*h>MAX_TILES||seconds<1||seconds>MAX_SECONDS||velocity<=0||velocity>10||count<0||count>MAX_EVENTS)return 0;cols=w;rows=h;duration=seconds;speed=velocity;event_count=count;random_state=seed;event_at=particle_count=collisions=exits=steps=0;sim_time=0;for(int i=0;i<MAX_SECONDS;i++)bins[i]=0;for(int i=0;i<count;i++){int x=(int)schedule_input[i*4+1],y=(int)schedule_input[i*4+2],entry=(int)schedule_input[i*4+3];double time=schedule_input[i*4];if(x<0||x>=w||y<0||y>=h||entry<0||entry>3||time<0||(i&&time<events[i-1].time))return 0;events[i]=(Event){time,x,y,entry};}for(int i=0;i<w*h;i++)if(tiles[i]>2)return 0;return 1;}
static void advance(void){if(steps>=duration*60)return;steps++;sim_time+=DT;if(sim_time>duration)sim_time=duration;while(event_at<event_count&&events[event_at].time<sim_time){Event*e=&events[event_at++];if(particle_count<MAX_PARTICLES){Particle*p=&particles[particle_count++];*p=(Particle){e->x,e->y,e->entry,exits_table[tiles[e->y*cols+e->x]][e->entry],0,0,0,0,0};}}
for(int i=0;i<particle_count;i++){Particle*p=&particles[i];p->dead=0;p->positioned=0;p->exit=exits_table[tiles[p->y*cols+p->x]][p->entry];p->u+=DT*speed/(((p->entry+2)%4==p->exit)?1:0.7853981633974483);if(p->u>=1){int e=p->exit;p->x+=dx[e];p->y+=dy[e];if(p->x<0||p->x>=cols||p->y<0||p->y>=rows){p->dead=1;exits++;continue;}p->entry=(e+2)%4;p->exit=exits_table[tiles[p->y*cols+p->x]][p->entry];p->u=0;}}
/* Stable pair order is part of the model. Tile broad phase avoids trig for distant pairs. */
for(int i=0;i<particle_count;i++){Particle*a=&particles[i];if(a->dead)continue;for(int j=0;j<i;j++){Particle*b=&particles[j];if(b->dead||a->x-b->x>1||b->x-a->x>1||a->y-b->y>1||b->y-a->y>1)continue;locate(a);locate(b);double x=a->px-b->px,y=a->py-b->py;if(x*x+y*y<.0441){int tx=(int)((a->px+b->px)/2),ty=(int)((a->py+b->py)/2);if(tx<0)tx=0;if(tx>=cols)tx=cols-1;if(ty<0)ty=0;if(ty>=rows)ty=rows-1;int idx=ty*cols+tx;tiles[idx]=(tiles[idx]+1+(int)(random_value()*2))%3;collisions++;int bin=(int)sim_time;if(bin>=duration)bin=duration-1;bins[bin]++;a->dead=b->dead=1;break;}}}
int write=0;for(int i=0;i<particle_count;i++)if(!particles[i].dead){if(write!=i)particles[write]=particles[i];write++;}particle_count=write;}
void step(int count){for(int i=0;i<count&&steps<duration*60;i++)advance();}
int collision_count(void){return collisions;}
int exit_count(void){return exits;}
int active_count(void){return particle_count;}
double time_value(void){return sim_time;}
void snapshot(void){for(int i=0;i<particle_count;i++){Particle*p=&particles[i];locate(p);double*out=&particle_output[i*7];out[0]=p->x;out[1]=p->y;out[2]=p->entry;out[3]=p->exit;out[4]=p->u;out[5]=p->px;out[6]=p->py;}}
/* Freestanding compiler helpers; avoid depending on a libc in WebAssembly. */
void*memset(void*p,int v,size_t n){volatile unsigned char*d=p;for(size_t i=0;i<n;i++)d[i]=(unsigned char)v;return p;}
void*memcpy(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
