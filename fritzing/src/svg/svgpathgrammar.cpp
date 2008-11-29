// This file was generated by qlalr - DO NOT EDIT!
#include "svgpathgrammar_p.h"

const char *const SVGPathGrammar::spell [] = {
  "end of file", 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 
#ifndef QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO
"path_data", "moveto_drawto_command_groups", "moveto_drawto_command_group", "moveto", "drawto_commands", "drawto_command", "closepath", 
  "lineto", "horizontal_lineto", "vertical_lineto", "curveto", "smooth_curveto", "quadratic_bezier_curveto", "smooth_quadratic_bezier_curveto", "moveto_command", "moveto_argument_sequence", "coordinate_pair", 
  "comma_wsp", "lineto_command", "lineto_argument_sequence", "horizontal_lineto_command", "horizontal_lineto_argument_sequence", "coordinate", "vertical_lineto_command", "vertical_lineto_argument_sequence", "curveto_command", "curveto_argument_sequence", 
  "curveto_argument", "smooth_curveto_command", "smooth_curveto_argument_sequence", "smooth_curveto_argument", "quadratic_bezier_curveto_command", "quadratic_bezier_curveto_argument_sequence", "quadratic_bezier_curveto_argument", "smooth_quadratic_bezier_curveto_command", "smooth_quadratic_bezier_curveto_argument_sequence", "x_coordinate", 
  "y_coordinate", "wspplus", "$accept"
#endif // QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO
};

const int SVGPathGrammar::lhs [] = {
  13, 14, 14, 15, 15, 17, 17, 18, 18, 18, 
  18, 18, 18, 18, 18, 16, 28, 28, 28, 20, 
  32, 32, 32, 21, 34, 34, 34, 22, 37, 37, 
  37, 23, 39, 39, 39, 40, 40, 40, 40, 24, 
  42, 42, 42, 43, 43, 25, 45, 45, 45, 46, 
  46, 26, 48, 48, 48, 29, 29, 49, 50, 30, 
  30, 51, 35, 27, 31, 33, 36, 38, 41, 44, 
  47, 19, 52};

const int SVGPathGrammar:: rhs[] = {
  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 
  1, 1, 1, 1, 1, 2, 1, 3, 2, 2, 
  1, 3, 2, 2, 1, 3, 2, 2, 1, 3, 
  2, 2, 1, 2, 3, 5, 4, 4, 3, 2, 
  1, 2, 3, 2, 3, 2, 1, 2, 3, 3, 
  2, 2, 1, 3, 2, 3, 2, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 2};


#ifndef QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO
const int SVGPathGrammar::rule_info [] = {
    13, 14
  , 14, 15
  , 14, 15, 14
  , 15, 16, 17
  , 15, 16
  , 17, 18
  , 17, 18, 17
  , 18, 19
  , 18, 20
  , 18, 21
  , 18, 22
  , 18, 23
  , 18, 24
  , 18, 25
  , 18, 26
  , 16, 27, 28
  , 28, 29
  , 28, 29, 30, 28
  , 28, 29, 28
  , 20, 31, 32
  , 32, 29
  , 32, 29, 30, 32
  , 32, 29, 32
  , 21, 33, 34
  , 34, 35
  , 34, 35, 30, 34
  , 34, 35, 34
  , 22, 36, 37
  , 37, 35
  , 37, 35, 30, 37
  , 37, 35, 37
  , 23, 38, 39
  , 39, 40
  , 39, 40, 39
  , 39, 40, 30, 39
  , 40, 29, 30, 29, 30, 29
  , 40, 29, 30, 29, 29
  , 40, 29, 29, 30, 29
  , 40, 29, 29, 29
  , 24, 41, 42
  , 42, 43
  , 42, 43, 42
  , 42, 43, 30, 42
  , 43, 29, 29
  , 43, 29, 30, 29
  , 25, 44, 45
  , 45, 46
  , 45, 46, 45
  , 45, 46, 30, 45
  , 46, 29, 30, 29
  , 46, 29, 29
  , 26, 47, 48
  , 48, 29
  , 48, 29, 30, 48
  , 48, 29, 48
  , 29, 49, 30, 50
  , 29, 49, 50
  , 49, 35
  , 50, 35
  , 30, 51
  , 30, 10
  , 51, 12
  , 35, 11
  , 27, 1
  , 31, 3
  , 33, 4
  , 36, 5
  , 38, 6
  , 41, 7
  , 44, 8
  , 47, 9
  , 19, 2
  , 52, 13, 0};

const int SVGPathGrammar::rule_index [] = {
  0, 2, 4, 7, 10, 12, 14, 17, 19, 21, 
  23, 25, 27, 29, 31, 33, 36, 38, 42, 45, 
  48, 50, 54, 57, 60, 62, 66, 69, 72, 74, 
  78, 81, 84, 86, 89, 93, 99, 104, 109, 113, 
  116, 118, 121, 125, 128, 132, 135, 137, 140, 144, 
  148, 151, 154, 156, 160, 163, 167, 170, 172, 174, 
  176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 
  196, 198, 200};
#endif // QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO

const int SVGPathGrammar::action_default [] = {
  0, 64, 5, 0, 2, 1, 0, 66, 68, 65, 
  69, 70, 71, 67, 72, 8, 12, 0, 6, 4, 
  10, 0, 9, 0, 14, 0, 13, 0, 15, 0, 
  11, 0, 63, 58, 0, 33, 32, 0, 61, 62, 
  0, 0, 60, 0, 0, 37, 36, 0, 39, 38, 
  0, 34, 35, 0, 59, 57, 56, 7, 25, 24, 
  0, 27, 26, 21, 20, 0, 23, 22, 0, 47, 
  46, 0, 51, 50, 0, 48, 49, 0, 41, 40, 
  0, 44, 45, 0, 42, 43, 53, 52, 0, 55, 
  54, 29, 28, 0, 31, 30, 17, 16, 0, 19, 
  18, 3, 73};

const int SVGPathGrammar::goto_default [] = {
  6, 5, 4, 2, 19, 18, 15, 22, 20, 30, 
  16, 26, 24, 28, 3, 97, 34, 40, 23, 64, 
  21, 59, 33, 31, 92, 17, 36, 35, 27, 79, 
  78, 25, 70, 69, 29, 87, 37, 55, 42, 0};

const int SVGPathGrammar::action_index [] = {
  14, -13, 25, -11, 0, -13, 2, -13, -13, -13, 
  -13, -13, -13, -13, -13, -13, -13, -11, 5, -13, 
  -13, -8, -13, -8, -13, -8, -13, -8, -13, -8, 
  -13, -8, -13, -13, -6, -6, -13, -6, -13, -13, 
  -8, -6, -13, -6, -8, -13, -13, -8, -13, -13, 
  -8, -13, -13, -8, -13, -13, -13, -13, -6, -13, 
  -8, -13, -13, -6, -13, -8, -13, -13, -6, -6, 
  -13, -8, -13, -13, -11, -13, -13, -6, -6, -13, 
  -11, -13, -13, -11, -13, -13, -6, -13, -8, -13, 
  -13, -6, -13, -11, -13, -13, -6, -13, -11, -13, 
  -13, -13, -13, 

  -40, -40, -40, -15, 35, -40, -40, -40, -40, -40, 
  -40, -40, -40, -40, -40, -40, -40, -40, -4, -40, 
  -40, -5, -40, -6, -40, -12, -40, -7, -40, -8, 
  -40, -9, -40, -40, -10, 5, -40, 23, -40, -40, 
  -1, 2, -40, 4, -2, -40, -40, -11, -40, -40, 
  -19, -40, -40, 6, -40, -40, -40, -40, 46, -40, 
  11, -40, -40, 33, -40, 18, -40, -40, 30, 39, 
  -40, -13, -40, -40, 10, -40, -40, 13, 44, -40, 
  -14, -40, -40, 22, -40, -40, 37, -40, 0, -40, 
  -40, 40, -40, 3, -40, -40, 60, -40, 8, -40, 
  -40, -40, -40};

const int SVGPathGrammar::action_info [] = {
  32, 1, 102, 32, 38, 32, 39, 14, 9, 7, 
  13, 8, 10, 11, 12, 1, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 14, 9, 7, 
  13, 8, 10, 11, 12, 0, 0, 0, 

  57, 96, 82, 73, 68, 49, 41, 52, 86, 77, 
  63, 0, 0, 91, 46, 43, 86, 58, 48, 47, 
  45, 44, 50, 100, 96, 91, 68, 95, 54, 81, 
  80, 51, 62, 58, 63, 90, 101, 67, 77, 0, 
  53, 0, 76, 56, 0, 54, 72, 71, 0, 63, 
  65, 85, 66, 86, 88, 68, 74, 93, 0, 0, 
  77, 83, 91, 60, 94, 0, 0, 61, 58, 0, 
  0, 75, 89, 84, 0, 99, 96, 98, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int SVGPathGrammar::action_check [] = {
  11, 1, 0, 11, 10, 11, 12, 2, 3, 4, 
  5, 6, 7, 8, 9, 1, -1, -1, -1, -1, 
  -1, -1, -1, -1, -1, -1, -1, 2, 3, 4, 
  5, 6, 7, 8, 9, -1, -1, -1, 

  4, 16, 16, 16, 16, 16, 16, 26, 16, 16, 
  16, -1, -1, 22, 16, 16, 16, 22, 16, 17, 
  16, 17, 17, 15, 16, 22, 16, 24, 22, 16, 
  17, 26, 21, 22, 16, 35, 1, 19, 16, -1, 
  17, -1, 32, 37, -1, 22, 16, 17, -1, 16, 
  17, 29, 19, 16, 17, 16, 17, 17, -1, -1, 
  16, 17, 22, 17, 24, -1, -1, 21, 22, -1, 
  -1, 32, 35, 29, -1, 15, 16, 17, -1, -1, 
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

