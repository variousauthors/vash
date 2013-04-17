vash
====

/* Andre Byrne
 * 100045589 */ 

OK so check this out it is awesome

try the following

$$ mk source /usr/bin
$$ source:ls
$$ source:pwd
$$ source:cd ..
$$ source:pwd

try it yourself: you can use any name in place of "source" and any dir

Also try something crazy like:

cd ..; ls > goo; wc -c < goo; cd ..; ls | wc | wc | less &

Also try something like:

vi &

(the results are kind of weird (see known issues) but it works)

Also try using ^C to terminate a process, but not Vash

ASSUMPTIONS: 

  1.) the PATH environment variable will not change during a session

  rational: I am going to parse the PATH once upon instantiating a VASH
            instance and only parse it again any time the user 
            explicitly demands that it be changed, via a builtin like 
            source.

  2.) other than the fork/exec calls, VASH is being run linearily in a
      single thread.

  rational: in many of the classes I have not copied data members passed 
            into constructors because of the linear nature of the execution
            of the program. For example, when creating a Context, I pass in 
            a List *, which is further passed to a Command constructor. I 
            do not copy the list because I am certain of the order of execution.
            The list will only be freed by me once the Command has been
            executed. 

            In later applications I may revise this approach. 

KNOWN ISSUES: 
  
  1.) Currently my program does not handle SIGCHLD. It has a little check
      before the prompt comes up that notices any children with "WNOHANG"
      so that it doesn't hang. 

  2.) if a function like wc is called into the background wc & it will want 
      to read standard input. I don't know how to handle this properly. I have
      rigged things up so that if the child process receives SIGTTOU or TTIN
      the parent will send it SIGKILL. As a result a process like wc & will
      look like this

      [] 4003
      [] 4003 stopped 22
      [] 4003 killed 9

      (note the [] are empty right now because I haven't implemented a table...)

      This is not the way I wanted to do it... But I just couldn't figure out
      signal handling!!! :(

  3.) unfortunately, my implementation of pipes doesn't work in conjunction 
      with redirection. For example,

      ls > goo | wc < goo

      in this case my program will essentially execute ls < goo > goo | wc 
      because of the order in which I parse parameters. Because pipes can be
      executed entirely in the background (that is to say, backgroundedness
      distributes over pipedness), I implement the entire pipeline as one 
      special case of the Command object. As a result, I loose the ability 
      to have seperate in/out files for each pipe segment. 

      In my defense, the above should really be 

      ls > goo ; wc < goo

      since there is no output from ls > goo. My program would handle that 
      case properly.

  4.) problems with builtin mk (make branch). I didn't notice this before 
      but apparently there are problems with make if you don't use absolute
      paths. 
    
      relative paths like .. work and so does . and /bin etc... but not
      lab03 or ./lab03 hmm... I'll fix that in a later revision.

  5.) MEMORY LEAK in command.c around line 267, I allocate an array. Then later
      in the code I fiddle with the pointer to the array. The reason is that 
      the single array is actually a number of NULL separated arrays. I take
      something like

      { "/bin/ls", "-a", "-l", "|", "/usr/bin/wc", "-c", "|", "/usr/bin/wc", NULL }

      and turn it into...

      { "/bin/ls", "-a", "-l", NULL, "/usr/bin/wc", "-c", NULL, "/usr/bin/wc", NULL }

      That way when I pass it to execv, with the pointer argv pointing to the first
      entry, execv will only read until the NULL. Then I adjust argv to point to
      just after the NULL. I am very careful about NULL dereferences ans out of 
      bounds errors, so I don't get those. Also, I save a pointer to the first 
      element. That way I can free it later (but apparently this doesn't work 
      sometimes). I wonder if free depends on NULL termination as well? That would
      explain it. Anyway, minor memory leak. 

      Note: I realize this is a bad lazy way to do things and it relies on 
      knowledge of how execv is implemented. What I should do is parse this,

      { "/bin/ls", "-a", "-l", "|", "/usr/bin/wc", "-c", "|", "/usr/bin/wc", NULL }

      into this:

      {
        { "/bin/ls", "-a", "-l", NULL }, 
        { "/usr/bin/wc", "-c", NULL }, 
        { "/usr/bin/wc", NULL }
      }

      I even tried to implement it that way, but I was a little stressed about 
      another error (which I've since fixed) and didn't feel like doing all
      the stuff I would have needed to do (counting the number of | in the 
      original, bounds checking, all that) especially since I already had a
      (bad lazy) solution that worked (mostly, most of the time). 

      Forgive me please!!!

  6.) Splint -weak gives me some warning about not assigning __pid_t to pid_t
      but I don't know what that's about. 
