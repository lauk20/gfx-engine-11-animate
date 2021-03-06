/*========== my_main.c ==========

  This is the only file you need to modify in order
  to get a working mdl project (for now).

  my_main.c will serve as the interpreter for mdl.
  When an mdl script goes through a lexer and parser,
  the resulting operations will be in the array op[].
  =========================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "parser.h"
#include "symtab.h"
#include "y.tab.h"

#include "matrix.h"
#include "ml6.h"
#include "display.h"
#include "draw.h"
#include "stack.h"
#include "gmath.h"

/*======== struct vary_node * get_knob_node() ==========
  Inputs: struct vary_node * front,
          char * knob_name
  Returns: Pointer to the knob node found in front

  Searches the list staring at front for a vary_node named knob_name.

  Returns a pointer to the node of NULL if not found.
  ====================*/
struct vary_node * get_knob_node(struct vary_node * front, char * knob_name) {
  struct vary_node * curr = front;

  while ( curr ) {
    if (strncmp( curr->name, knob_name,
                 sizeof(curr->name)) == 0 ) {
      return curr;
    } //end found
    curr = curr->next;
  }//end while
  return curr;
}

/*======== struct vary_node * add_node() ==========
  Inputs: struct vary_node * front,
          char * knob_name
  Returns: Pointer to the newly created node

  Creates a new vary_node with knob_name and a value of 0.

  Sets next of the new node to point to front.
  ====================*/
struct vary_node * add_node(struct vary_node * front, char * knob_name) {
  struct vary_node * curr = (struct vary_node*)calloc(1, sizeof(struct vary_node));
  strncpy( curr->name, knob_name, sizeof(curr->name));
  curr->value = 5;
  curr->next = front;
  return curr;
}

/*======== void first_pass() ==========
  Inputs:
  Returns:

  Checks the op array for any animation commands
  (frames, basename, vary)

  Should set num_frames and basename if the frames
  or basename commands are present

  If vary is found, but frames is not, the entire
  program should exit.

  If frames is found, but basename is not, set name
  to some default value, and print out a message
  with the name being used.
  ====================*/
void first_pass() {
  //These variables are defined at the bottom of symtab.h
  extern int num_frames;
  extern char name[128];

  int vary = 0;
  int frames = 0;
  int basename = 0;

  for (int i = 0; i < lastop; i++){
    if (op[i].opcode == VARY){
      vary = 1;
    } else if (op[i].opcode == FRAMES){
      frames = 1;
      num_frames = op[i].op.frames.num_frames;
    } else if (op[i].opcode == BASENAME){
      basename = 1;
      strcpy(name, op[i].op.basename.p->name);
    }
  }

  if (vary && !frames){
    exit(-1);
  }

  if (frames && !basename){
    printf("Basename: image\n");
  }

  if (!frames){
    num_frames = 1;
  }

}

/*======== struct vary_node ** second_pass() ==========
  Inputs:
  Returns: An array of vary_node linked lists

  In order to set the knobs for animation, we need to keep
  a seaprate value for each knob for each frame. We can do
  this by using an array of linked lists. Each array index
  will correspond to a frame (eg. knobs[0] would be the first
  frame, knobs[2] would be the 3rd frame and so on).

  Each index should contain a linked list of vary_nodes, each
  node contains a knob name, a value, and a pointer to the
  next node.

  Go through the opcode array, and when you find vary, go
  from knobs[0] to knobs[frames-1] and add (or modify) the
  vary_node corresponding to the given knob with the
  appropirate value.
  ====================*/
struct vary_node ** second_pass() {

  struct vary_node *curr = NULL;
  struct vary_node **knobs;
  knobs = (struct vary_node **)calloc(num_frames, sizeof(struct vary_node *));

  for (int i = 0; i < lastop; i++){
    if (op[i].opcode == VARY){
      int start_frame = op[i].op.vary.start_frame;
      int end_frame = op[i].op.vary.end_frame;
      int start_val = op[i].op.vary.start_val;
      int end_val = op[i].op.vary.end_val;
      char * knobname = op[i].op.vary.p->name;

      //printf("%d %d %d %d\n", start_frame, end_frame, start_val, end_val);

      double increment = ((double)(end_val - start_val)/(double)(end_frame - start_frame));
      double current = start_val - increment;
      for (int v = start_frame; v <= end_frame; v++){
        struct vary_node * new_node = add_node(knobs[v], knobname);
        //printf("%faa\n", ((double)(end_val - start_val)/(double)(end_frame - start_frame)));
        new_node->value = current + increment;
        current = new_node->value;
        /*
        printf("KN%p\n", knobs[v]);
        printf("NT%p\n", new_node->next);
        */
        knobs[v] = new_node;
        /*
        printf("nn%p\n", new_node);
        printf("kn%p\n", knobs[v]);
        printf("nnn%p\n", new_node->next);
        */
        //printf("%s %f\n", knobname, new_node->value);
      }
    }
  }

  return knobs;
}


void my_main() {

  struct vary_node ** knobs;
  struct vary_node * vn;
  first_pass();
  knobs = second_pass();
  char frame_name[128];
  int f;

  int i;
  struct matrix *tmp;
  struct stack *systems;
  screen t;
  zbuffer zb;
  double step_3d = 100;
  double theta, xval, yval, zval;

  //Lighting values here for easy access
  color ambient;
  ambient.red = 50;
  ambient.green = 50;
  ambient.blue = 50;

  double light[2][3];
  light[LOCATION][0] = 0.5;
  light[LOCATION][1] = 0.75;
  light[LOCATION][2] = 1;

  light[COLOR][RED] = 255;
  light[COLOR][GREEN] = 255;
  light[COLOR][BLUE] = 255;

  double view[3];
  view[0] = 0;
  view[1] = 0;
  view[2] = 1;

  //default reflective constants if none are set in script file
  struct constants white;
  white.r[AMBIENT_R] = 0.1;
  white.g[AMBIENT_R] = 0.1;
  white.b[AMBIENT_R] = 0.1;

  white.r[DIFFUSE_R] = 0.5;
  white.g[DIFFUSE_R] = 0.5;
  white.b[DIFFUSE_R] = 0.5;

  white.r[SPECULAR_R] = 0.5;
  white.g[SPECULAR_R] = 0.5;
  white.b[SPECULAR_R] = 0.5;

  //constants are a pointer in symtab, using one here for consistency
  struct constants *reflect;
  reflect = &white;

  color g;
  g.red = 255;
  g.green = 255;
  g.blue = 255;


  systems = new_stack();
  tmp = new_matrix(4, 1000);
  clear_screen( t );
  clear_zbuffer(zb);

  for (int v = 0; v < num_frames; v++){
    struct vary_node * current = knobs[v];
    while (current){
      SYMTAB * knob_symbol = lookup_symbol(current->name);
      if (knob_symbol){
        //printf("SET%f %s\n", current->value, current->name);
        set_value(knob_symbol, current->value);
      } else {
        add_symbol(current->name, SYM_VALUE, &(current->value));
      }
      current = current->next;
    }

    for (i=0;i<lastop;i++) {

      //printf("%d: ",i);
      switch (op[i].opcode)
        {
        case SPHERE:
          /*printf("Sphere: %6.2f %6.2f %6.2f r=%6.2f",
                 op[i].op.sphere.d[0],op[i].op.sphere.d[1],
                 op[i].op.sphere.d[2],
                 op[i].op.sphere.r);*/
          if (op[i].op.sphere.constants != NULL) {
            //printf("\tconstants: %s",op[i].op.sphere.constants->name);
            reflect = lookup_symbol(op[i].op.sphere.constants->name)->s.c;
          }
          if (op[i].op.sphere.cs != NULL) {
            //printf("\tcs: %s",op[i].op.sphere.cs->name);
          }
          add_sphere(tmp, op[i].op.sphere.d[0],
                     op[i].op.sphere.d[1],
                     op[i].op.sphere.d[2],
                     op[i].op.sphere.r, step_3d);
          matrix_mult( peek(systems), tmp );
          draw_polygons(tmp, t, zb, view, light, ambient,
                        reflect);
          tmp->lastcol = 0;
          reflect = &white;
          break;
        case TORUS:
          /*printf("Torus: %6.2f %6.2f %6.2f r0=%6.2f r1=%6.2f",
                 op[i].op.torus.d[0],op[i].op.torus.d[1],
                 op[i].op.torus.d[2],
                 op[i].op.torus.r0,op[i].op.torus.r1);*/
          if (op[i].op.torus.constants != NULL) {
            //printf("\tconstants: %s",op[i].op.torus.constants->name);
            reflect = lookup_symbol(op[i].op.sphere.constants->name)->s.c;
          }
          if (op[i].op.torus.cs != NULL) {
            //printf("\tcs: %s",op[i].op.torus.cs->name);
          }
          add_torus(tmp,
                    op[i].op.torus.d[0],
                    op[i].op.torus.d[1],
                    op[i].op.torus.d[2],
                    op[i].op.torus.r0,op[i].op.torus.r1, step_3d);
          matrix_mult( peek(systems), tmp );
          draw_polygons(tmp, t, zb, view, light, ambient,
                        reflect);
          tmp->lastcol = 0;
          reflect = &white;
          break;
        case BOX:
          /*printf("Box: d0: %6.2f %6.2f %6.2f d1: %6.2f %6.2f %6.2f",
                 op[i].op.box.d0[0],op[i].op.box.d0[1],
                 op[i].op.box.d0[2],
                 op[i].op.box.d1[0],op[i].op.box.d1[1],
                 op[i].op.box.d1[2]);*/
          if (op[i].op.box.constants != NULL) {
            //printf("\tconstants: %s",op[i].op.box.constants->name);
            reflect = lookup_symbol(op[i].op.sphere.constants->name)->s.c;
          }
          if (op[i].op.box.cs != NULL) {
            //printf("\tcs: %s",op[i].op.box.cs->name);
          }
          add_box(tmp,
                  op[i].op.box.d0[0],op[i].op.box.d0[1],
                  op[i].op.box.d0[2],
                  op[i].op.box.d1[0],op[i].op.box.d1[1],
                  op[i].op.box.d1[2]);
          matrix_mult( peek(systems), tmp );
          draw_polygons(tmp, t, zb, view, light, ambient,
                        reflect);
          tmp->lastcol = 0;
          reflect = &white;
          break;
        case LINE:
          /*printf("Line: from: %6.2f %6.2f %6.2f to: %6.2f %6.2f %6.2f",
                 op[i].op.line.p0[0],op[i].op.line.p0[1],
                 op[i].op.line.p0[2],
                 op[i].op.line.p1[0],op[i].op.line.p1[1],
                 op[i].op.line.p1[2]);*/
          if (op[i].op.line.constants != NULL) {
            //printf("\n\tConstants: %s",op[i].op.line.constants->name);
          }
          if (op[i].op.line.cs0 != NULL) {
            //printf("\n\tCS0: %s",op[i].op.line.cs0->name);
          }
          if (op[i].op.line.cs1 != NULL) {
            //printf("\n\tCS1: %s",op[i].op.line.cs1->name);
          }
          add_edge(tmp,
                   op[i].op.line.p0[0],op[i].op.line.p0[1],
                   op[i].op.line.p0[2],
                   op[i].op.line.p1[0],op[i].op.line.p1[1],
                   op[i].op.line.p1[2]);
          matrix_mult( peek(systems), tmp );
          draw_lines(tmp, t, zb, g);
          tmp->lastcol = 0;
          break;
        case MOVE:
        ;
          xval = op[i].op.move.d[0];
          yval = op[i].op.move.d[1];
          zval = op[i].op.move.d[2];
          /*printf("Move: %6.2f %6.2f %6.2f",
                 xval, yval, zval);*/
          if (op[i].op.move.p != NULL) {
            //printf("\tknob: %s",op[i].op.move.p->name);
            xval = op[i].op.move.d[0] * op[i].op.move.p->s.value;
            yval = op[i].op.move.d[1] * op[i].op.move.p->s.value;
            zval = op[i].op.move.d[2] * op[i].op.move.p->s.value;
            /*printf("Move: %6.2f %6.2f %6.2f",
                   xval, yval, zval);*/
          }
          tmp = make_translate( xval, yval, zval );
          matrix_mult(peek(systems), tmp);
          copy_matrix(tmp, peek(systems));
          tmp->lastcol = 0;
          break;
        case SCALE:
          xval = op[i].op.scale.d[0];
          yval = op[i].op.scale.d[1];
          zval = op[i].op.scale.d[2];
          /*printf("Scale: %6.2f %6.2f %6.2f",
                 xval, yval, zval);*/
          if (op[i].op.scale.p != NULL) {
            //printf("\tknob: %s",op[i].op.scale.p->name);
            xval = op[i].op.scale.d[0] * op[i].op.scale.p->s.value;
            yval = op[i].op.scale.d[1] * op[i].op.scale.p->s.value;
            zval = op[i].op.scale.d[2] * op[i].op.scale.p->s.value;
            /*printf("Scale: %6.2f %6.2f %6.2f",
                   xval, yval, zval);*/
          }
          tmp = make_scale( xval, yval, zval );
          matrix_mult(peek(systems), tmp);
          copy_matrix(tmp, peek(systems));
          tmp->lastcol = 0;
          break;
        case ROTATE:
          theta =  op[i].op.rotate.degrees * (M_PI / 180);
          /*printf("Rotate: axis: %6.2f degrees: %6.2f",
                 op[i].op.rotate.axis,
                 theta);*/
          if (op[i].op.rotate.p != NULL) {
            //printf("\tknob: %s",op[i].op.rotate.p->name);
            //printf("VAL%f\n", op[i].op.rotate.p->s.value);
            theta =  op[i].op.rotate.degrees * (M_PI / 180) * op[i].op.rotate.p->s.value;
            /*printf("Rotate: axis: %6.2f degrees: %6.2f",
                   op[i].op.rotate.axis,
                   theta);*/
          }

          if (op[i].op.rotate.axis == 0 )
            tmp = make_rotX( theta );
          else if (op[i].op.rotate.axis == 1 )
            tmp = make_rotY( theta );
          else
            tmp = make_rotZ( theta );

          matrix_mult(peek(systems), tmp);
          copy_matrix(tmp, peek(systems));
          tmp->lastcol = 0;
          break;
        case PUSH:
          //printf("Push");
          push(systems);
          break;
        case POP:
          //printf("Pop");
          pop(systems);
          break;
        case SAVE:
          printf("Save: %s\n",op[i].op.save.p->name);
          save_extension(t, op[i].op.save.p->name);
          break;
        case DISPLAY:
          //printf("Display");
          display(t);
          break;
        case FRAMES:
          //printf("Frames");
          break;
        case BASENAME:
          //printf("Basename");
          break;
        case VARY:
          //printf("Vary");
          break;
        }
      //printf("\n");
    } //end operation loop

    if (num_frames > 1){
      char filename[128];
      strcpy(filename, "anim/");
      strcat(filename, name);
      char number[128];
      sprintf(number, "%03d", v);
      strcat(filename, number);
      save_extension(t, filename);

      printf("Saved Frame: %s\n", filename);
      //display(t);

      free_stack(systems);
      systems = new_stack();
      free_matrix(tmp);
      tmp = new_matrix(4, 1000);
      clear_screen( t );
      clear_zbuffer(zb);
    }
  }

  free_stack( systems );
  free_matrix( tmp );

  if (num_frames > 1){
    make_animation(name);
  }
}
