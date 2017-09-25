#ifndef __MACRO_H__
#define __MACRO_H__

#define STATE_NULL              -1
#define STATE_SUCCESSFUL        true
#define STATE_FAILED            false

/*
 * Define constants for the floor field.
 */
#define INIT_WEIGHT             FLT_MAX
#define EXIT_WEIGHT             0.f
#define OBSTACLE_WEIGHT         5000.f

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

#endif