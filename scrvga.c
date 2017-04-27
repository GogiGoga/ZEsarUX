//gcc -lvga -o scrvga.o scrvga.c
//gcc -lvgagl -lvga -o scrvga.o scrvga.c
#include <stdio.h>
#include <vga.h>
//#include <vgagl.h>
#include <stdlib.h>
/*
   G320x200x16(1),       G640x200x16(2),      G640x350x16(3),
       G640x480x16(4),     G320x200x256(5),      G320x240x256(6),
       G320x400x256(7), G360x480x256(8), and G640x480x2(9)

   Basic SVGA modes
       These  use  linear  256  color  memory  layouts similar to
       G320x200x256.

       G640x480x256(10), G800x600x256(11), G1024x768x256(12), and
       G1280x1024x256(13)

 #include <vga.h> defines vga_modeinfo as

       typedef struct {
            int width;
            int height;
            int bytesperpixel;
            int colors;
            int linewidth;
            int maxlogicalwidth;
            int startaddressrange;
            int maxpixels;
            int haveblit;
            int flags;
       /* Extended fields, not always available:
            int chiptype;
            int memory;
            int linewidth_unit;
            char *linear_aperture;
            int aperture_size;
            void (*set_aperture_page) (int page);
            void *extensions;
       } vga_modeinfo;

*/

//GraphicsContext physicalscreen;

unsigned char *p;
vga_modeinfo *v;

int main(void)
{

  int x,y;
  int c=0;

  if (vga_init()) {
    printf ("Error al iniciar modo grafico!\a");
    return -1;
  }
  v=vga_getmodeinfo(G320x240x256);
if (v!=NULL) {
  printf ("Bytes por pixel: %d\n",v->bytesperpixel);
  if (vga_hasmode(G320x240x256)==0) printf ("modo no disponible\n");
}
  getchar();

  if (vga_setmode(G320x240x256)==-1) { //320*400*256
    printf ("Error al seleccionar modo grafico!\a");
    return -1;
  }
  //gl_setcontextvga(G320x400x256);

  //gl_setcontextvgavirtual(G320x400x256);
  //gl_getcontext(&physicalscreen);
/*  if (vga_setlinearaddressing()==-1) {
    printf ("Error al activar modo lineal\a");
    return -1;
  }
  printf ("Hola desde modo lineal");
  getchar();*/

  vga_setpalette(7,0,0,30); //RGB

  for (x=0;x<320;x++)
    for (y=0;y<200;y++) {
      vga_setcolor(7);
      vga_drawpixel(x,y);
    }


  //gl_copyscreen(&physicalscreen);
  /*vga_setpage(1);
  vga_setpalette(0,0,0,0); //RGB
  vga_setpalette(1,30,0,0); //RGB
  vga_setpalette(2,0,30,0); //RGB
  vga_setpalette(3,0,0,30); //RGB
  p=vga_getgraphmem();
  for (x=0;x<320;x++) *p++=(c++)%4;
*/

  /*vga_setpage(1);
  p=vga_getgraphmem();
  for (x=0;x<65536;x++) *p++=rand()%256;*/

  getchar();
}
