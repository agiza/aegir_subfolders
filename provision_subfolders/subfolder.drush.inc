<?php

/**
 * @file
 *   The subfolder provision service.
 */

/**
 * Expose the service type this extension defines to provision.
 *
 * @return
 *   An array with the service type the key, and the default implementation the value.
 */
function subfolder_provision_services() {
  return array('subfolder' => NULL);
}

/**
 * The subfolder service base class.
 */
class provisionService_subfolder extends provisionService {
  public $service = 'subfolder';

  /**
   * Add the subfolder_path property to the site context.
   */
  static function subscribe_site($context) {
    $context->setProperty('subfolder_path');
  }
}

function drush_subfolder_pre_provision_install() {
  drush_subfolder_pre_provision_verify();
}

function drush_subfolder_pre_provision_verify() {
  # Create a stub.d directory in the /var/aegir/config area
  # that is *not* pre-loaded/included by Aegir, but is included
  # by our platform's apache config at the right time
  # This would otherwise be injected directly through
  # subfolder_provision_apache_dir_config(), however we want
  # to separate per subsite, and we only want to call RewriteEngine once
  # within an <IfModule> wrapper. Why? I don't know, but it works.
  $stubdir = d()->server->config_path . '/apache/stub.d';
  provision_file()->mkdir($stubdir)
    ->succeed('Created directory @path.')
    ->fail('Could not create directory @path.');

  # Create the symlink necessary for multisite subfolders to work
  # Symlinks the subfolder name into the top-level of the Drupal root itself
  if (!empty(d()->subfolder_path)) {
    provision_file()->symlink(d()->root, d()->subfolder_path)
      ->succeed('Created symlink @path to @target')
      ->fail('Could not create symlink @path to @target: @reason');
  }
}

/**
 * Inject a snippet that instructs the platform-level Apache config to include
 * our special RewriteRules per server across this platform, *before* the .htaccess
 * is included. Order seems to be important here - the only other option is to 
 * hack the .htaccess directly otherwise.
 */
function subfolder_provision_apache_dir_config($data = null) {
  $lines = array();
  $lines[] = '<IfModule mod_rewrite.c>';
  $lines[] = 'RewriteEngine On';
  $lines[] = 'Include ' . d()->server->http_pred_path . '/*.stub';
  $lines[] = '</IfModule>';
  return implode($lines, "\n");
}


/**
 * Implementation of hook_provision_apache_vhost_config().
 *
 * Nasty hack that will insert our RewriteRule overrides per
 * site into a .stub file which the platform-level Apache config includes.
 * Also is used to stick an Alias one-liner into the server-wide config space.
 * @TODO This should probably not go here, since we aren't actually 
 * injecting into the site's vhost, we're just abusing an opportunity
 * to do stuff at the right time in the API.
 */
function subfolder_provision_apache_vhost_config($uri, $data=null) {
  $lines = array();
  if (d()->type == 'site') {
    if (!empty(d()->subfolder_path)) {
      # Nasty: stick an Alias one-liner for this site in a server-wide config (this can't go
      # in the .stub file, because that .stub file gets Included within a <Directory> directive,
      # which can't contain Alias directives. 
      # I think it also needs to be included *before* the pre.d etc. Otherwise, it could go in
      # to the server-wide apache config via hook_provision_apache_server_config()
      $alias_stub = d()->server->http_pred_path . '/000_' . d()->subfolder_path . '_alias.conf';
      file_put_contents($alias_stub, 'Alias /' . d()->subfolder_path . ' ' . d()->root);

      # Inject the database credentials based on Request URI, since we can't rely on the Host header
      $lines[] = 'SetEnvIf Request_Uri ^/'. d()->subfolder_path . ' db_type=' . $data['db_type'];
      $lines[] = 'SetEnvIf Request_Uri ^/'. d()->subfolder_path . ' db_name=' .  $data['db_name'];
      $lines[] = 'SetEnvIf Request_Uri ^/'. d()->subfolder_path . ' db_user=' . $data['db_user'];
      $lines[] = 'SetEnvIf Request_Uri ^/'. d()->subfolder_path . ' db_passwd=' . $data['db_passwd'];
      $lines[] = 'SetEnvIf Request_Uri ^/'. d()->subfolder_path . ' db_host=' . $data['db_host'];
      $lines[] = 'SetEnvIf Request_Uri ^/'. d()->subfolder_path . ' db_port=' . $data['db_port'];

      # This is the magic that makes the subfolders work, if the symlink is also present in the doc root
      # It's an override of the default .htaccess
      $lines[] = 'RewriteCond %{REQUEST_FILENAME} !-f';
      $lines[] = 'RewriteCond %{REQUEST_FILENAME} !-d';
      $lines[] = 'RewriteCond %{REQUEST_URI} !=/favicon.ico';
      $lines[] = 'RewriteCond %{REQUEST_URI} ^/' . d()->subfolder_path;
      $lines[] = 'RewriteRule ^ /' . d()->subfolder_path . '/index.php';
    
      # Write out our stub 
      $stubdir = d()->server->config_path . '/apache/stub.d/';
      $config = $stubdir . d()->subfolder_path . '.stub';
      file_put_contents($config, implode($lines, "\n"));
    }
  }
}
