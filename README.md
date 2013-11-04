Arduino Trimet

This is a program I use with my Arduino Mega, 3 DE-DP14112 from Sure Electronics, and a python script running
out on my website.

trimet_update.py contains the parsing needed to pull data out of trimet's website.

You'll need to add your API key, in addition to your route numbers and bus lines you care about.

Point url and path in StreetcarTrimet to where the parsed data is served from.


Simply set trimet_update.py to be executable on your server, and add a cron job to delete the old file and
re-run the script.  My server has:

0 * * * * /bin/rm /var/www/trimet; /home/cdunbar/trimet_update.py


Required Libraries:
https://github.com/amcewen/HttpClient
https://github.com/charlesdunbar/libraries/tree/master/ht1632c
http://playground.arduino.cc/uploads/Code/Time.zip
