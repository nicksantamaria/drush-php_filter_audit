# Drush Block Benchmark

Quick and dirty drush plugin to check a drupal implementation for code processed by PHP filter format.

## Usage

1. [Download](https://github.com/nicksantamaria/drush-php_filter_audit/archive/master.zip) this to plugin to `${HOME}/.drush`
1. Run `drush cc drush`
1. From your sites docroot, run `drush php_filter_audit`

It will output any areas of the site which it detects having PHP code.

## Audit Details

This script checks the following components of the site

- Checks if `php.module` is enabled.
- Checks all fields for any values using the `php_code` filter format.
- Checks views display configuration for php code.
- Checks if block visibility rules use php code.
- Checks if custom block content uses php code.

## Roadmap

The following checks are planned for the future, please open an issue if you have more ideas.

- Panels
