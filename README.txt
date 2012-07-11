'Subfolder' support for Aegir.
------------------------------

This is really ugly, but it works and doesn't require any use of mod_proxy that would otherwise
double-up requests.

Installation instructions
------------------------

Move provision_subfolders to /var/aegir/.drush/

Move hosting_subfolders to somewhere like /var/aegir/hostmaster-6.x-1.x/sites/all/yoursite/modules/

Enable the 'Hosting subfolders' module at yoursite.com/admin/build/modules/

Usage instructions
------------------

1. Go to /node/add/site/ and give your site some unique main URL as always (can be a dummy URL that
doesn't even resolve in DNS, if you want)

2. Specify a 'subfolder'. No opening or trailing slashes

3. You *must* give your site a site URL Alias via the 'Site aliases' feature, that is consistent
with the multisite/subfolder instructions for Drupal.
(e.g for example.com/foobar, add an alias of example.com.foobar)


Caveats
--------

I'm presuming you've picked some 'main' URL that you want your sites to be a subfolder of, e.g
'example.com'

This 'example.com' must exist as a vhost on your server somewhere, e.g be a Drupal site already, or
even something as small as this in your /etc/apache2/conf.d/example.com.conf:

<VirtualHost *:80>
  ServerName example.com
</VirtualHost>

If you don't have a valid 'ServerName' vhost for this site on the server, then you'll get 404s for
your subsites due to this line in /var/aegir/config/server_master/apache.conf:

<VirtualHost *:80>
  ServerName default
  Redirect 404 /
</VirtualHost>

So add a vhost, or comment out the 'Redirect 404 /' here.

This Aegir extension does not support Nginx at all, at this stage.
