#
#  Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
#  Created by:
#     Anton Berezin  <tobez@plab.ku.dk>
#     Dmitry Karasik <dk@plab.ku.dk> 
#
#  $Id$
package Prima::Const;
use Prima '';
use Carp;

sub AUTOLOAD {
    my ($pack,$constname) = ($AUTOLOAD =~ /^(.*)::(.*)$/);
    my $val = eval "\&${pack}::constant(\$constname)";
    croak $@ if $@;
    *$AUTOLOAD = sub () { $val };
    goto &$AUTOLOAD;
}

package nt; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# notification types
package kb; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# keyboard-related constants
package km; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# keyboard modifiers
package mb; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# mouse buttons & message box constants
package ta; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# text alignment
package cl; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# colors
package ci; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# color indices
package wc; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# widget classes
package cm; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# commands
package rop; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# raster operations
package gm; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# grow modes
package lp; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# pen styles
package fp; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# fill styles & font pitches
package le; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# line ends
package fs; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# font subtypes
package fw; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# font weights
package bi; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# border icons
package bs; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# border styles
package ws; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# window states
package sv; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# system values
package im; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# image types
package ict; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# Image conversion types
package is; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# Image statistics types
package am; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# Icon auto masking
package apc; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# OS type
package gui; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# GUI types
package dt; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# Drives types & draw_text constants
package cr; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# Pointer ( Cursor Resources) id's
package sbmp; *AUTOLOAD =\&Prima::Const::AUTOLOAD;	# System bitmaps index
package hmp; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# help manager pages constants
package tw; *AUTOLOAD =  \&Prima::Const::AUTOLOAD;	# text wrapping constants
package fds; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# find/replace dialog scope type
package fdo; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# find/replace dialog options
package fe; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# file events constants
package fr; *AUTOLOAD = \&Prima::Const::AUTOLOAD;	# fetch resource constants

1;
