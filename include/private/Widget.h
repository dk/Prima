#ifndef Widget_private_H_
#define Widget_private_H_

#ifdef __cplusplus
extern "C" {
#endif

#undef my
#define inherited CDrawable
#define my  ((( PWidget) self)-> self)
#define var (( PWidget) self)
#define his (( PWidget) child)

extern Handle
Widget_check_in( Handle self, Handle in, Bool barf);

extern void
Widget_grid_slaves( Handle self);

extern void
Widget_pack_slaves( Handle self);

extern void
Widget_place_slaves( Handle self);

extern Bool
Widget_size_notify( Handle self, Handle child, const Rect* metrix);

extern Bool
Widget_move_notify( Handle self, Handle child, Point * moveTo);

extern void
Widget_grid_enter( Handle self);

extern void
Widget_grid_leave( Handle self);

extern void
Widget_grid_destroy( Handle self);


#ifdef __cplusplus
}
#endif
#endif
