package X11::XRemote;

use 5.006;
use strict;
use warnings;
use Carp qw(cluck croak);

require Exporter;
require DynaLoader;

our @ISA = qw(Exporter DynaLoader);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use X11::XRemote ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );
our @EXPORT_OK   = ( @{ $EXPORT_TAGS{'all'} } );
our @EXPORT      = qw( );
our $VERSION     = '0.01';
our $DEBUG_VALUE = 0;
my $PING_COMMAND = 'ping';

bootstrap X11::XRemote $VERSION;

#==========================================================#
#  SUBS: Call directly, do not send to an object or class  #
#==========================================================#
sub x_program{
    my ($x_program) = @_;
    if ($x_program) {
        _x_program_set($x_program);
    }
    return _x_program_get();
}

#==========================================================#
# ALL MODES SUBS: Avaiable to client and server            #
#==========================================================#
sub new ($){
    my ( $class, @args) = @_;
    my $p = {};
    $p = { @args } if(!(scalar(@args) % 2));
    my $self = { 
        '_id'            => 0,
        '_server'        => 0,
        '_request_list'  => [], 
        '_response_list' => [], 
        '_handle'        => undef,
        '__DEBUG'        => $DEBUG_VALUE,
        map { "_".lc(substr($_,1)) => $p->{$_} } keys(%$p)
    };
    bless($self, $class);
    if(my $xrh = X11::XRemote::Handle::init_obj($class)){
        $self->_handle($xrh);
        my $id = $self->{'_id'} if $self->{'_id'};
        if($self->{'_server'} && $id){
            my $app = $self->application("perl");
            $xrh->initialiseServer($app, oct($id));
            $xrh->setRequestName($self->{'_request_name'}) 
                if $self->{'_request_name'};
            $xrh->setResponseName($self->{'_response_name'})
                if $self->{'_response_name'};
        }elsif($id){
            $self->window_id($id);
        }
        return $self;
    }else{
        croak(qq`$!`);
    }
    return 0;
}
sub window_id($;$){ 
    my ($self, $id) = @_;
    if($id){
        # check it looks like a window id, i.e. oct will return > 0
        if(!($self->{'_server'})){
            $self->_handle->window_id(oct($id));
            $self->_handle->initialiseClient(oct($id));
            $self->{'_id'} = $id;
        }
    }
    return $self->{'_id'};
}
sub request_name{
    my ($self, $name) = @_;
    return unless $name;
    __DEBUG($self->{'__DEBUG'}, "request $name");
    $self->_handle->setRequestName($name);
}
sub response_name{
    my ($self, $name) = @_;
    return unless $name;
    __DEBUG($self->{'__DEBUG'}, "response $name");
    $self->_handle->setResponseName($name);
}
sub is_error{
    my ($self, $response) = @_;
    return 0 unless $response;
    my $del = $self->delimiter();
    my ($status, $xml) = split(/$del/, $response, 2);
    return ($status =~ /^20\d$/ ? 0 : 1);
}

#==========================================================#
# CLIENT ONLY MODE SUBS: available only for client modes   #
#==========================================================#
sub send_commands{
    my ($self, @commands) = @_;
    $self->{'_response_list'} = [];
    $self->{'_request_list'}  = [];

    return (qq{503:<xml>\n}                                  .
            qq{\t<error>\n}                                  .
            qq{\t\t<message>servers may not send commands. } .
            qq{command not sent. }                           .
            qq{\t\t</message>\n}                             .
            qq{\t</error>\n}                                 .
            qq{</xml>}) x @commands if $self->_is_server();
    
    if(blocked()){
        cluck( __PACKAGE__ . ": Avoided race condition." );
        return (qq{500:<xml>\n}                                   .
                qq{\t<error>\n}                                   .
                qq{\t\t<message>you cannot send while receiving } .
                qq{until you have replied. }                      .
                qq{See perldoc } . __PACKAGE__                    .
                qq{\t\t</message>\n}                              .
                qq{\t</error>\n}                                  .
                qq{</xml>}) x @commands;
    }
    # Ok everything is good
    foreach my $cmd(@commands){
        __DEBUG($self->{'__DEBUG'}, $cmd);
        eval{
            push(@{$self->{'_request_list'}}, $cmd);
            $self->_int_send_command($cmd);
        };
        my $reply = $@ || $self->_getResponse();
        push(@{$self->{'_response_list'}}, $reply);
    }
    return @{$self->{'_response_list'}};
}
sub ping{
    my ($self) = @_;
    my $resp = ($self->send_commands($PING_COMMAND))[0];
    if($self->is_error($resp)){
        return 0 if $resp =~ /bad\s?window/i;
        return 0 if $resp =~ /not a valid/i;
        return 0 if $resp =~ /window was destroyed/i;
        return 0 if $resp =~ /packed up and gone home/i;
    }
    return 1;
}
#==========================================================#
# SERVER ONLY MODE SUBS: available for server modes only   #
#==========================================================#
sub request_string{
    my ($self) = @_;
    return unless $self->_is_server();
    my $req = $self->{'_current_request'};
    if(!$req){
        $req = $self->_handle->request();
        $self->{'_current_request'} = $req;
    }else{
        my $tmp = $self->_handle->request();
        if($tmp && $tmp ne $req){
            $req = $tmp;
        }
        $self->{'_current_request'} = $req;
    }
    return $req;
}
sub application{
    my ($self, $app) = @_;
    $self->{'_application'} ||= $app if $app;
    return $self->{'_application'};
}

{
    # BLOCK should only be available here!!!
    # Nothing should be allowed to reset it except send_reply()
    my $BLOCK = 0;

    sub block{
        $BLOCK = 1;
    }
    sub blocked{
        return ($BLOCK ? 1 : undef);
    }

    sub send_reply{
        my ($self, $reply) = @_;
        return unless $self->_is_server();
        local *unblock = sub { 
            $BLOCK = 0; # man perlref says this should work
        };
        $self->_handle->reply($reply);
        unblock();
    }
}
#==========================================================#
#                   INTERNALS                              #
#==========================================================#
sub _int_send_command(){
    my ($self, $command) = @_;
    __DEBUG($self->{'__DEBUG'}, $command);
    return unless $self->window_id(); # needs a window id
    $self->_handle->send_command($command);    
}
sub _getResponse{
    my ($self) = @_;
    my $response = $self->_handle->full_response_text();
    __DEBUG($self->{'__DEBUG'}, $response);
    # THIS SHOULD BE XML!!!!
    # 200:<xml><magnification>100</magnification></xml>
    # 30X:<xml><error>Page Moved</error></xml>
    # 40X:<xml><error>you crazy fool</error></xml>
    # 50X:<xml><error>I'm a crazy fool</error></xml>
    return $response;
}
sub _handle{
    my ($self, $hdle) = @_;
    if($hdle){
        $self->{'_handle'} || ($self->{'_handle'} = $hdle);
    }
    return $self->{'_handle'};
}
sub _is_server{
    my ($self) = @_;
    return ($self->{'_server'} ? 1 : 0);
}
sub __DEBUG{
    return unless shift;
    my @c = (caller(1))[0..3];
    warn "($c[1]) LINE: $c[2], says: @_\n";
}
#==========================================================#
#                  DESTROY                                 #
#==========================================================#
sub DESTROY{
    # REALLY THINK ABOUT THIS
    my ($self) = @_;
    warn "Destroying $self ..." if $self->{'__DEBUG'};
}

#==========================================================#
1;      # TRUE.                                            #                                                   
__END__ # The END...                                       #
#==========================================================#

# Below is stub documentation for your module. You better edit it!
# Ok I'll do that, just in case I get run over by a bus.

=head1 NAME

X11::XRemote - Perl extension for telling people to do stuff.

=head1 SYNOPSIS

  use X11::XRemote;
  my $x = X11::XRemote->new([options]); # see below for options
  my @commands = qw(command1 command2);
  my @replies  = $x->send_commands(@commands);

=head1 DESCRIPTION

Every wanted to remote control an application, but only ever found the
packet of chocolate bourbon biscuits.  Well here's a module that might
help. Assuming of course that it's a remote control TV.

The idea rests on the XChangeProperty method in Xlib and having an app
which  takes notice of  a change  to the  property.  Moreover  the app
which is being controlled must set a reply atom when it's received the
Event,  otherwise this  module  will  wait for  it...   It's based  on
Netscape/Mozilla remote.c and some code written by Fred Wobus of AceDB
fame (see comments in C code).

It seems  to work, but as  ever no representations are  made about the
suitability of this software for  any purpose.  It is provided "as is"
without express or implied warranty.

=head1 USAGE - Object Interface

=head1 COMMON MODE METHODS

=head2 new([-option => "value"])

Returns a new  object of the style requested.   The style is specified
using the following options.  Please  note that for the server mode it
is recommended  that you specify these  in the new  method rather than
after initialisation.

=over 10

=item I<-server> '0|1'

Specifies whether to create a server or a client.

=item I<-id> window id

Specifies the window id you are using  in the case of a server, or the
one you want to control in the case of a client. E.g 0x123456

=item I<-request_name> atom name

Specifies the name  of the atom which will  receive the requests. (Set
Only)

=item I<-response_name> atom name

Specifies the name  of the atom which will  contain the response. (Set
Only)

=back

=head2 window_id($windowID)

Set the  window id of the remote  window.  It is only  possible to use
this method to set the window id for the client mode.  If you're using
the server mode use (-id => $windowID) in the new method.

=head2 request_name($requestName)

Set the name of the atom which will receive the requests.

=head2 response_name($responseName)

Set the name of the atom which will contain the response.

=head2 is_error($response)

Returns True (1) if the $response is non 200, False otherwise...

=head1 CLIENT MODE METHODS

=head2 send_commands(@commands)

Send the commands to the remote window.  This will _NOT_ work with the
server version!

=head1 SERVER MODE METHODS

=head2 request_string(Z<>)

Get the contents of the request atom.

=head2 send_reply(Z<>)

This  sets the contents  of the  response atom.  ONLY works  in server
mode!

=head1 SUBROUTINES

All kinds of clever methods...come from the XS layer.

=head2 format_string(Z<>)

A string which  can be put into sprintf() to format  the reply for the
server mode.

=head2 delimiter(Z<>)

A string which should occur  in the I<format_string()> and can be used
to split the reply string.

=head2 version(Z<>)

A string which looks remarkablly similar to the version number.

=head2 client_request_name(Z<>)

A string which corresponds to the name of the default request atom the
client should use.

=head2 client_response_name(Z<>)

A string  which corresponds to the  name of the  default response atom
the client should use.

=head2 x_program(Z<>)

Gets the X program name.

=head2 x_program($x_program)

Sets the X program name.

=head1 AUTHOR

R.Storey <rds@sanger.ac.uk>

=head1 SEE ALSO

L<perl>.

=cut
