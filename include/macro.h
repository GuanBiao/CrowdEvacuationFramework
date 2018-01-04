#ifndef __MACRO_H__
#define __MACRO_H__

#define STATE_NULL              -1

/*
 * Define constants for the floor field.
 */
#define INIT_WEIGHT             5000.f
#define EXIT_WEIGHT             0.f
#define OBSTACLE_WEIGHT         FLT_MAX
#define UPDATE_STATIC           0
#define UPDATE_DYNAMIC          1
#define UPDATE_BOTH             2

/*
 * Define cell states.
 */
#define TYPE_EMPTY              -1
#define TYPE_EXIT               -2
#define TYPE_MOVABLE_OBSTACLE   -3
#define TYPE_IMMOVABLE_OBSTACLE -4
#define TYPE_AGENT              -5

/*
 * Define flags for exit editing.
 */
#define DIR_HORIZONTAL          1
#define DIR_VERTICAL            2

/*
 * Define flags for the movable obstacle map.
 */
#define STATE_IDLE              -2
#define STATE_DONE              -3

/*
 * Define game types.
 */
#define GAME_YIELDING           0
#define GAME_VOLUNTEERING       1

#endif