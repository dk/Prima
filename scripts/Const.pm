#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
package Const;
use Prima '';
use Carp;

sub AUTOLOAD {
    my ($pack,$constname) = ($AUTOLOAD =~ /^(.*)::(.*)$/);
    my $val = eval "\&${pack}::constant(\$constname)";
    croak $@ if $@;
    *$AUTOLOAD = sub () { $val };
    goto &$AUTOLOAD;
}

package nt; *AUTOLOAD = \&Const::AUTOLOAD;	# notification types
package kb; *AUTOLOAD = \&Const::AUTOLOAD;	# keyboard-related constants
package km; *AUTOLOAD = \&Const::AUTOLOAD;	# keyboard modifiers
package mb; *AUTOLOAD = \&Const::AUTOLOAD;	# mouse buttons & message box constants
package ta; *AUTOLOAD = \&Const::AUTOLOAD;	# text alignment
package cl; *AUTOLOAD = \&Const::AUTOLOAD;	# colors
package ci; *AUTOLOAD = \&Const::AUTOLOAD;	# color indices
package wc; *AUTOLOAD = \&Const::AUTOLOAD;	# widget classes
package cm; *AUTOLOAD = \&Const::AUTOLOAD;	# commands
package rop; *AUTOLOAD = \&Const::AUTOLOAD;	# raster operations
package gm; *AUTOLOAD = \&Const::AUTOLOAD;	# grow modes
package lp; *AUTOLOAD = \&Const::AUTOLOAD;	# pen styles
package fp; *AUTOLOAD = \&Const::AUTOLOAD;	# fill styles & font pitches
package le; *AUTOLOAD = \&Const::AUTOLOAD;	# line ends
package fs; *AUTOLOAD = \&Const::AUTOLOAD;	# font subtypes
package fw; *AUTOLOAD = \&Const::AUTOLOAD;	# font weights
package bi; *AUTOLOAD = \&Const::AUTOLOAD;	# border icons
package bs; *AUTOLOAD = \&Const::AUTOLOAD;	# border styles
package ws; *AUTOLOAD = \&Const::AUTOLOAD;	# window states
package sv; *AUTOLOAD = \&Const::AUTOLOAD;	# system values
package im; *AUTOLOAD = \&Const::AUTOLOAD;	# image types
package ict; *AUTOLOAD = \&Const::AUTOLOAD;	# Image conversion types
package is; *AUTOLOAD = \&Const::AUTOLOAD;	# Image statistics types
package apc; *AUTOLOAD = \&Const::AUTOLOAD;	# OS type
package gui; *AUTOLOAD = \&Const::AUTOLOAD;	# GUI types
package dt; *AUTOLOAD = \&Const::AUTOLOAD;	# Drives types & draw_text constants
package cr; *AUTOLOAD = \&Const::AUTOLOAD;	# Pointer ( Cursor Resources) id's
package sbmp; *AUTOLOAD = \&Const::AUTOLOAD;	# System bitmaps index
package hmp; *AUTOLOAD = \&Const::AUTOLOAD;	# help manager pages constants
package tw; *AUTOLOAD = \&Const::AUTOLOAD;	# text wrapping constants
package fds; *AUTOLOAD = \&Const::AUTOLOAD;	# find/replace dialog scope type
package fdo; *AUTOLOAD = \&Const::AUTOLOAD;	# find/replace dialog options

1;
