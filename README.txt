This is DumbStreamer, the dumb streamer!
As such, do not expect it to be smart...

Here is a quick'n'dirty, informal, description about how to compile
and test the thing (do not expect to find any information about what
the dumb streamer is, or how it works. If you need such information,
please drop me an email - luca.abeni@unitn.it):

Ok, here we go...
- To download the DumbStreamer
	git clone http://www.disi.unitn.it/~abeni/PublicGits/DumbStreamer.git
- To compile it:
	cd DumbStreamer
	make prepare
	make
   You can type "make FFDIR=<path to an ffmpeg build>" to compile a version
   of the streamer with a minimal libav-based input module.
   You can type "make DEBUG=WhyNot" to compile a "debug" version of the
   dumb streamer (prints out a lot of crap and debug information, not really
   useful in practice unless you are trying to hunt a bug).
   You can type "make THREADS=YesPlease" to compile the multi-thread version
   of the streamer. Not tested with libav*.
   You can type "make GRAPES=<path to your grapes>" to use a different build
   of GRAPES
- To test it:
     First, I suggest to test the non-libav-based version. It generates
     "fake" text chunks, which are useful for debugging
	1) build as above (make prepare; make)
	2) start a source: ./dumbstreamer
	3) in a different shell, start a client: ./dumbstreamer -P 5555 -i 127.0.0.1 -p 6666
	4) start another client: ./dumbstreamer -P 5556 -i 127.0.0.1 -p 6666
	5) ...
     Explanation: "-P <port>" is the local port used by the client (6666 by
     default). Since I am testing source and multiple clients on the same
     machine, every client has to use a different port number. "-i <IP addr>"
     is the source IP, and "-p <port>" is the port number used by the source.
     After this testing, I suggest recompiling with debug on, and with threads.
     Test as above, and enjoy.
- To see something interesting:
     You have to build a libav-based version:
	1) cd /tmp; svn checkout svn://svn.ffmpeg.org/ffmpeg/trunk ffmpeg
	2) cd ffmpeg; ./configure; make -j 3; cd ..
	3) git clone http://www.disi.unitn.it/~abeni/PublicGits/DumbStreamer.git
	4) cd DumbStreamer; make prepare; make FFDIR=/tmp/ffmpeg
     Now, prepare a (video-only) input file:
	5) /tmp/ffmpeg -i <whatever.avi> -r 25 -an -vcodec mpeg4 -f m4v test.m4v
         6) The input currently needs to be called "input.mpg", so
            ln -s test.m4v input.mpg
     Start the source. For the moment, you need to manually specify the rate
     (will be fixed in the future):
	7) ./dumbstreamer -t 40 -c 50
     create a FIFO for the output, and attach a player to it:
	8) mkfifo out; ffplay out
     start a client:
	9) ./dumbstreamer -P 5555 -p 6666 -i 127.0.0.1 > out
- Enjoy...  ;-) 

A lot of cleanup is needed, and the input module is still far from being
reasonable (but this is not our business  ;-) 
But you can now enjoy some video...

Remember! The DumbStreamer is dumb! Peers try to send chunks to the source...
Scheduling is blind...
All of this is very bandwidth inefficient. If you want to start more clients,
please increase the upload bandwidth of the source (dumbstreamer -t 40 -c 200,
or even more...).


			Enjoy your streaming,
				Luca
