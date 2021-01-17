### UDP based file content transfer

Run client as `./client hostname filename`.

The server looks for the file, if not found errors and exits.

If found, the contents of file are sent line by line to client and the client writes them down in a new file called `client_file.txt`.
