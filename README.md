# practiceWebserver
A simple webserver created with winsock32 as the base for the networking. 

TO USE: must create a folder "webroot" in the CMAKE directory. Place the files you would like to display in there.

The goal of the project is to have a webserver capable of handling requests for files, reading the files from disk, then sending the contents to the user's browser.
By completion, the webserver should have working thread-safe multithreading using pthread and a cache with a limit of 5 files
The webserver will respond to requests like this: http://localhost:8888/filename
also responds to ip requests: 10.129.45.64:8888/filename
it should be able to parse the filename and search the disk for that file
for that, there will be a webroot folder in the project files that it will look through for the filename
