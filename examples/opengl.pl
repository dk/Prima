=pod

=head1 NAME

OpenGL example

=head1 DESCRIPTION

The program demonstrates the use of the OpenGL lighting model.  A icosahedron
is drawn using a grey material characteristic.  A single light source
illuminates the object. Example adapted from light.c.

The code demonstrates use of Prima together with OpenGL . Currently ( OpenGL
v0.64 ) it can only operate on X-based systems with GLX support, and only with
one GL window per application.  The original example code can be found in
OpenGL distribution in examples/light.pl .

=cut

use strict;
use warnings;
use Prima qw(Application Buttons);
use OpenGL qw(:glfunctions :glconstants :glxfunctions :glxconstants);

my $use_lighting  = 1;
my $use_frame     = 1;
my $use_rotation  = 1;

my $spin          = 0;
my $grab          = 0;
my $frame_color   = 1;
my $top;

sub icosahedron
{
	# from OpenGL Programming Guide page 56
	my $x = 0.525731112119133606;
	my $z = 0.850650808352039932;

	my @v = (
		[-$x,	0,  $z],
		[ $x,	0,  $z],
		[-$x,	0, -$z],
		[ $x,	0, -$z],
		[  0,  $z,  $x],
		[  0,  $z, -$x],
		[  0, -$z,  $x],
		[  0, -$z, -$x],
		[ $z,  $x,   0],
		[-$z,  $x,   0],
		[ $z, -$x,   0],
		[-$z, -$x,   0],
	);

	my @t = (
		[0, 4, 1],	[0, 9, 4],
		[9, 5, 4],	[4, 5, 8],
		[4, 8, 1],	[8, 10, 1],
		[8, 3, 10],	[5, 3, 8],
		[5, 2, 3],	[2, 7, 3],
		[7, 10, 3],	[7, 6, 10],
		[7, 11, 6],	[11, 0, 6],
		[0, 1, 6],	[6, 1, 10],
		[9, 0, 11],	[9, 11, 2],
		[9, 2, 5],	[7, 2, 11],
	);

	for ( my $i = 0; $i < 20; $i++) {
		glBegin(GL_POLYGON);
		for ( my $j = 0; $j < 3; $j++) {
			$use_lighting || glColor3f(0,$i/19.0,$j/2.0);
			glNormal3f( @{$v[$t[$i][$j]]});
			glVertex3f( @{$v[$t[$i][$j]]});
		}
		glEnd();

		if( $use_frame){
			glPushAttrib(GL_ALL_ATTRIB_BITS);
			glDisable(GL_LIGHTING);
			glColor3f($frame_color,0,0);
			glBegin(GL_LINE_LOOP);
			glVertex3f( map { 1.01 * $_ } @{$v[$_]}) for @{$t[$i]};
			glEnd();
			glPopAttrib();
		}
	}
}

sub init
{
	if ( $use_lighting ) {
		# Initialize material property, light source, lighting model, 
		# and depth buffer.
		my @mat_specular = ( 1.0, 1.0, 0.0, 1.0 );
		my @mat_diffuse  = ( 0.0, 1.0, 1.0, 1.0 );
		my @light_position = ( 1.0, 1.0, 1.0, 0.0 );
		
		glMaterialfv_s(GL_FRONT, GL_DIFFUSE, pack("f4",@mat_diffuse));
		glMaterialfv_s(GL_FRONT, GL_SPECULAR, pack("f4",@mat_specular));
		glMaterialf(GL_FRONT, GL_SHININESS, 10);
		glLightfv_s(GL_LIGHT0, GL_POSITION, pack("f4",@light_position));
		
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glDepthFunc(GL_LESS);
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
	}
} 

sub display
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	glRotatef(23*sin($spin*3.14/180),1,0,0);
	glRotatef($spin,0,1,0);
	if ( $grab ) {
		my ( $x, $y ) = $top-> pointerPos;
		glRotatef( $x, 0, 1, 0);
		glRotatef( $y, 1, 0, 0);
	}		
	icosahedron;
	glPopMatrix();
	
	glFlush();
	glXSwapBuffers();
}

sub reshape
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.5, 1.5, -1.5, 1.5, -10.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

$top = Prima::MainWindow-> new(
	size => [ 300, 300 ],
	text => 'OpenGL example',
	menuItems => [
		['~Options' => [
			['*' => '~Rotate' => sub { 
				$use_rotation = $_[0]-> menu-> toggle( $_[1] );
			}],
			['*' => '~Lightning' => sub { 
				$use_lighting = $_[0]-> menu-> toggle( $_[1] );
				init();
			}],
			['*' => '~Frame' => sub { 
				$use_frame = $_[0]-> menu-> toggle( $_[1] );
				init();
			}],
		]],
	],

	onCreate => sub {
		OpenGL::glpOpenWindow(
			width      => 1,
			height     => 1, 
			steal      => 1,
			parent     => eval shift-> get_client_handle,
			attributes => [
				GLX_DOUBLEBUFFER,
				GLX_RGBA,
				GLX_DEPTH_SIZE, 16,
				GLX_RED_SIZE,   1,
				GLX_GREEN_SIZE, 1,
				GLX_BLUE_SIZE,  1
			]);
		init();
		reshape();
		glEnable(GL_DEPTH_TEST);
		glRotatef(0.12,1,0,0);
	},
	onSize => sub {
		my ( $self, $ox, $oy, $x, $y ) = @_;
  		glViewport(0,0,$x,$y);
	},
	onPaint      => \&display,
	onMouseDown  => sub { $grab = 1 },
	onMouseUp    => sub { $grab = 0 },
);

$top-> insert( Timer => 
	timeout => 5,
	onTick  => sub {
		$spin++ if $use_rotation and not $grab;
		$frame_color = 1 if ($frame_color -= 0.005) < 0;
		display;
	}
)-> start;

$top-> insert( Button => 
	origin  => [ 5, 5 ],
	text    => '~Quit',
	onClick => sub { $::application-> close },
);

run Prima;
