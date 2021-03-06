<?php

/**
 * Implementation of hook_drush_command().
 */
function php_filter_audit_drush_command() {
  $items['php_filter_audit'] = array(
    'description' => 'Scans Drupal instance for php filter content.',
  );
  return $items;
}

/**
 * Drush command callback for Block Benchmark.
 */
function drush_php_filter_audit() {
  _php_filter_audit_run_phpmodule();

  drush_print('Checking field API for PHP code.');
  _php_filter_audit_run_fields();

  if (module_exists('views')) {
    drush_print('Checking views config for PHP code.');
    _php_filter_audit_run_views();
  }

  drush_print('Checking blocks for PHP code.');
  _php_filter_audit_run_blocks();
}

/**
 * Helper function which checks for php.module and dependent modules.
 */
function _php_filter_audit_run_phpmodule() {
  if (module_exists('php')) {
    drush_print('php.module is enabled - checking for enabled dependent modules.');

    $module_data = system_rebuild_module_data();
    foreach ($module_data['php']->required_by as $dependent => $dependent_data) {
      if (module_exists($dependent)) {
        drush_print(' - ' . $dependent . '.module is enabled and depends on php.module.');
      }
    }

  }
  else if ($path = drupal_get_path('module', 'php')) {
    drush_print('php.module is not enabled, but is still in the Drupal codebase. It is recommended to delete the module directory: "' . $path . '"');
  }
  else {
    drush_print('php.module is not present in the codebase.');
  }
}

/**
 * Helper function which checks fields content for php code.
 */
function _php_filter_audit_run_fields() {
  $fields = field_info_fields();
  $fields_to_check = array();
  foreach ($fields as $field_name => $info) {
    if (isset($info['foreign keys']['format'])) {
      $fields_to_check[$field_name] = $field_name;
    }
  }

  if (!empty($fields_to_check)) {
    foreach ($fields_to_check as $field_name) {
      $q = new EntityFieldQuery();
      $q->fieldCondition($field_name, 'format', _php_filter_audit_get_formats(), 'IN');
      $rows = $q->execute();
      if (!empty($rows)) {
        drush_print(format_string(' - PHP code detected in !field_name', array(
          '!field_name' => $field_name,
        )));
      }
    }
  }
  else {
    drush_print(' - No PHP code detected!');
  }
}

/**
 * Helper function which checks views config for php code.
 */
function _php_filter_audit_run_views() {
  $query = db_select('views_display', 'vd');
  $query->fields('vd', array());

  // Check 1: Check for text using filter formats.
  $or = db_or();
  foreach (_php_filter_audit_get_formats() as $format) {
    $or->condition('vd.display_options', '%' . db_like($format) . '%', 'LIKE');
  }

  // Check 2: Check for handlers using php code.
  $or->condition('vd.display_options', '%' . db_like('"php"') . '%', 'LIKE');
  $or->condition('vd.display_options', '%' . db_like('"code"') . '%', 'LIKE');

  $query->condition($or);
  $result = $query->execute();

  $views_detected = array();
  foreach ($result as $display) {
    $view = db_query("SELECT name FROM {views_view} WHERE vid = :id", array(':id' => $display->vid))->fetch();
    $views_detected[$view->name][] = $display->id;
  }

  if (!empty($views_detected)) {
    foreach ($views_detected as $view_name => $displays) {
      drush_print(format_string(' - PHP code detected in view "!view", displays: !displays.', array(
        '!view' => $view_name,
        '!displays' => implode(', ', $displays),
      )));
    }
  }
  else {
    drush_print(' - No PHP code detected!');
  }
}

/**
 * Helper function which checks blocks for php code.
 */
function _php_filter_audit_run_blocks() {
  // Searching for block visibility rules in PHP format.
  $result = db_query("SELECT * FROM {block} WHERE pages LIKE '%<?php%'");
  foreach ($result->fetchAll() as $row) {
    drush_print(format_string(' - PHP code detected in block visibility rules for module "!module", delta "!delta", theme "!theme".', array(
      '!module' => $row->module,
      '!delta' => $row->delta,
      '!theme' => $row->theme,
    )));
  }

  // Check custom block content.
  $result = db_query("SELECT * FROM {block_custom} WHERE format IN (:formats)", array(
    ':formats' => _php_filter_audit_get_formats(),
  ));
  foreach ($result->fetchAll() as $row) {
    drush_print(format_string(' - PHP code detected in custom block "!info", delta "!delta".', array(
      '!info' => $row->info,
      '!delta' => $row->bid,
    )));
  }
}

/**
 * Returns list of input formats using the php_code filter.
 */
function _php_filter_audit_get_formats() {
  $formats = array();

  $result = db_query("SELECT DISTINCT format FROM {filter} WHERE module = 'php' AND name = 'php_code'");
  foreach ($result->fetchAll() as $row) {
    $formats[$row->format] = $row->format;
  }

  return $formats;
}
