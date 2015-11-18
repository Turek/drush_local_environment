<?php
/**
 * @file
 * Provide Drush integration for release building and dependency building.
 */

/**
 * Use variable to force cache clear, where required,
 * even if no-cache-clear option is used. Defaults to false.
 */
$force_all_cache_clear = FALSE;

/**
 * Implements hook_drush_help().
 */
function local_env_drush_help($section) {
  switch ($section) {
    case 'drush:local-environment':
      return dt('Rebuild your local environment set up to Catch\'s standards.');
  }
}

/**
 * Implements hook_drush_command().
 */
function local_env_drush_command() {
  $items = array();

  $items['local-environment'] = array(
    'description' => 'Rebuild your local environment set up to Catch\'s standards.',
    'callback' => 'drush_local_env',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    // 'options' => array(
    //   'no-cache-clear' => 'Rebuild the registry only, do not clear caches, unless --fire-bazooka is also used.',
    //   'fire-bazooka' => 'Truncate registry and registry_file tables and build them from scratch. Forces all caches clear.',
    // ),
    // 'examples' => array(
    //   'drush rr --no-cache-clear' => 'Rebuild the registry only, do not clear caches, unless --fire-bazooka is also used.',
    //   'drush rr --fire-bazooka' => 'Truncate registry and registry_file tables and build them from scratch. Forces all caches clear.',
    // ),
    'aliases' => array('local-env'),
  );

  return $items;
}

/**
 * Rebuild the registry.
 *
 * Before calling this we need to be bootstrapped to DRUPAL_BOOTSTRAP_DATABASE.
 */
function drush_local_env() {
  ini_set('memory_limit', -1);
  if (!drush_bootstrap_to_phase(DRUSH_BOOTSTRAP_DRUPAL_DATABASE)) {
    return drush_set_error('DRUPAL_SITE_NOT_FOUND', dt('You need to specify an alias or run this command within a drupal site.'));
  }

  // Define paths.
  $default = DRUPAL_ROOT . DIRECTORY_SEPARATOR . 'sites' . DIRECTORY_SEPARATOR . 'default';
  $private = DRUPAL_ROOT . DIRECTORY_SEPARATOR . 'sites' . DIRECTORY_SEPARATOR . 'default' . DIRECTORY_SEPARATOR . 'private';
  $files = DRUPAL_ROOT . DIRECTORY_SEPARATOR . 'sites' . DIRECTORY_SEPARATOR . 'default' . DIRECTORY_SEPARATOR . 'files';

  // Define all files to be included.
  $includes = array(
    DRUPAL_ROOT . DIRECTORY_SEPARATOR . 'includes' . DIRECTORY_SEPARATOR . 'bootstrap.inc',
    DRUPAL_ROOT . DIRECTORY_SEPARATOR . 'includes' . DIRECTORY_SEPARATOR . 'file.inc',
  );

  // Load all needed files.
  foreach ($includes as $include) {
    if (file_exists($include)) {
      require_once($include);
    }
  }

  // Sometimes sites/default subdirectory is not set to 755, ensure it is.
  chmod($default, 0755);

  // Create sites/default/files subdirectory if it doesn't exist.
  if (!file_exists($files)) {
    mkdir($files, 2775, TRUE);
    drush_log(dt('Creating sites/default/files subdirectory.'), 'success');
  }

  // Create sites/default/private subdirectory if it doesn't exist.
  if (!file_exists($private)) {
    mkdir($private, 2775, TRUE);
    drush_log(dt('Creating sites/default/private subdirectory.'), 'success');
  }

  // Get apache user.
  $user = drush_shell_exec('ps axho user,group,comm|grep -E "httpd|apache"|uniq|grep -v "root"|awk \'END {if ($1 && $2) print $1}\'');
  $user = current(drush_shell_exec_output());
  drush_log(dt('Apache user: ' . $user), 'status');

  // Get first group of apache user.
  $groups = drush_shell_exec('id -Gn ' . $user);
  $groups = current(drush_shell_exec_output());
  $groups = explode(' ', $groups);
  $group = current($groups);
  drush_log(dt('Apache group: ' . $group), 'status');

  // Change permissions properly to files and private subdirectories.
  drush_shell_exec('nohup chown -Rv ' . $user . ':' . $group . ' ' . $files . '  > /dev/null');
  drush_shell_exec('nohup chown -Rv ' . $user . ':' . $group . ' ' . $private . ' > /dev/null');
  drush_shell_exec('nohup chgrp -Rv ' . $group . ' ' . $files . '  > /dev/null');
  drush_shell_exec('nohup chgrp -Rv ' . $group . ' ' . $private . ' > /dev/null');
  drush_shell_exec('nohup chmod -R 2775 ' . $files . '  > /dev/null');
  drush_shell_exec('nohup chmod -R 2775 ' . $private . ' > /dev/null');
  drush_shell_exec('nohup chmod -R g+w ' . $files . '  > /dev/null');
  drush_shell_exec('nohup chmod -R g+w ' . $private . ' > /dev/null');

  // Update paths for private and files subdirectories.
  $private = str_replace(DRUPAL_ROOT . DIRECTORY_SEPARATOR, '', $private);
  if (drush_confirm(dt('Do you want to update "file_private_path" to "' . $private . '"'))) {
    drush_variable_set('file_private_path', $private);
  }
  $files = str_replace(DRUPAL_ROOT . DIRECTORY_SEPARATOR, '', $files);
  if (drush_confirm(dt('Do you want to update "file_public_path" to "' . $files . '"'))) {
    drush_variable_set('file_public_path', $files);
  }
  if (drush_confirm(dt('Do you want to update "file_temporary_path" to "/tmp"'))) {
    drush_variable_set('file_temporary_path', '/tmp');
  }

  // variables to set:
  // file_temporary_path: '/tmp'
  // file_private_path: 'sites/default/files/private'
}
