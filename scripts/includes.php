#!/usr/bin/php
<?php
/*
includes.php, mh17 Feb 2010
Copyright (c) 2010 Genome research Ltd

Check the source tree for C files that include headers they should not

use find to get all the header files and note nested includes using grep
process header path name to get file and path
use find to get all the C files and list included headers using grep
ignore file names not beginning with zmap
report:
      files included more than once
      *_I.h and *_P.h included from another directory

NB: Only deals with include as <file> not "file"
We assume the file names are unique
-> they must be as they are included without directory names (except for include/ZMap)
*/

// NB: run from the ZMap source directory

/*
$strict = FALSE;
for($i = 1;$i < $argc;$i++)
{
      $arg = $argv[$i];
      if($arg == "-strict")
            $strict = TRUE;
      else
      {
           print ("error in arg $arg\n");
           exit;
      }
}
*/

$headers = array();


function get_includes($file)
{
      $inc = array();
      $cmd = "grep \"#include\" $file";
      exec($cmd,$ifiles = array());
      foreach($ifiles as $f)
      {
            $name = explode('>',$f);
            $name = explode('<',$name[0]);
            $name = $name[1];
            $name = explode('/',$name);
            $name = array_pop($name);
            if(!strncmp($name,"zmap",4))
            {
                  $inc[$name] = 1;
            }
      }
      return($inc);
}


// check that included files do not violate design modularity
// private headers are not to be included from other directories
function vet($file,$dir,$inc)
{
      global $headers;

      foreach($inc as $f => $x)
      {
            if(strpos($f,"_I") !== FALSE || strpos($f,"_P") !== FALSE)  // private header
            {
                  $head = $headers[$f];
                  $hd = $head['dir'];

                  if(strcmp($hd,$dir))
                  {
                        print("$dir/$file includes $hd/$f\n");
                  }
            }
      }
}




// find all the headers and the files they include
exec('find . -name "*.h"',$h_path = array());

foreach($h_path as $h)
{
      $names = explode("/",$h);
      $file = array_pop($names);
      $dir = implode("/",$names);
      if(!strncmp($file,"zmap",4))
      {
            $headers[$file] = array('dir' => $dir);
            $headers[$file]['includes'] = get_includes($h);
      }
}

// find the nested headers in each header

foreach($headers as $file => $head)
{
      vet($file,$head['dir'],$head['includes']);
}


// find all the source files and the headers they include

exec('find . -name "*.c"',$c_path = array());

foreach($c_path as $c)
{
      $names = explode("/",$c);
      $file = array_pop($names);
      $dir = implode("/",$names);

      $inc = get_includes($c);
      vet($file,$dir,$inc);
}


