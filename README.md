# Pantheon Drupal Composer Project

This is an unadvertised, unsupported fork of drupal-composer/drupal-project.

This project keeps the Drupal Root at the Repository root, as required by Pantheon. The composer.json file is relocated to the 'private' directory, and the Composer install paths are set as relative paths starting from the 'private' directory, starting with '../' to navigate up to the Drupal Root / Repository root.

The project otherwise behaves very similar to drupal-composer/drupal-project, which it is based on.

To use this:

1. Make a copy of this lean repository to call your own ("github-mysite").
2. Make a Pantheon site ("mysite").
3. Clone your Pantheon site locally:
  - git clone ssh://codeserver.dev...d@codeserver.dev....drush.in:2222/~/repository.git pantheon-mysite
  - cd pantheon-mysite
4. Add github-mysite as a second remote 
  - git remote add origin git@github.com:MY-ORG/github-mysite.git
5. Pull the master branch of your lean repository into a branch called 'lean'
  - git fetch github master:lean
6. Merge your lean repo files on top of your Pantheon site
  - git merge -s recursive -Xtheirs lean -m "Merge in initial upstream commit."
7. Set up your composer.json to suit, and then run `composer install`. n.b. Only commit composer.lock.
  - cd private
  - composer install
  - git add composer.lock
  - git commit -m "Add composer.lock file"

The general idea is that we will use GitHub webhooks to move commits made on the lean GitHub repository to Pantheon.  To test:

- terminus auth login
- secrets pantheon-mysite dev lean-repo https://github.com/MY-ORG/github-mysite.git
- secrets pantheon-mysite dev lean-gh-token YOUR-GITHUB-TOKEN-HERE
- wget http://dev-mysite.pantheon.io/webhooks/github.php

You can [download the 'secrets' script](https://github.com/pantheon-systems/quicksilver-examples/blob/16c010990579d8a15bbb6705dba463131b423ff4/scripts/secrets) from GitHub.

The call to wget should take the latest HEAD of master of your github-mysite repository, overlay it on top of your Pantheon repository, and call 'composer install'.  Note that 'composer update' requires too much RAM to run on Pantheon; you must run updates elsewhere, and then commit the lockfile to your repository.

TODO: Next we will set up a GitHub webhook to call our webhook on every commit to the repository. After that, we will put together an easier setup procedure, hopefully involving a simple call to `composer create-project` that prints instructions on how to push to your lean GitHub repository.

This is a work in progress; it is being committed so that end-to-end testing, including project creation may be done. Testing now in progress; works a little, but does not work all the way.  Probably will work soon.  Look for a more permanent home in the future at pantheon-systems/drupal-project.

Lean repository work based on https://github.com/joshkoenig/lean-and-mean.git.


## Original README for Composer template for Drupal projects

[![Build Status](https://travis-ci.org/drupal-composer/drupal-project.svg?branch=8.x)](https://travis-ci.org/drupal-composer/drupal-project)

This project template should provide a kickstart for managing your site
dependencies with [Composer](https://getcomposer.org/).

If you want to know how to use it as replacement for
[Drush Make](https://github.com/drush-ops/drush/blob/master/docs/make.md) visit
the [Documentation on drupal.org](https://www.drupal.org/node/2471553).

## Usage

First you need to [install composer](https://getcomposer.org/doc/00-intro.md#installation-linux-unix-osx).

> Note: The instructions below refer to the [global composer installation](https://getcomposer.org/doc/00-intro.md#globally).
You might need to replace `composer` with `php composer.phar` (or similar) 
for your setup.

After that you can create the project:

```
composer create-project drupal-composer/drupal-project:8.x-dev some-dir --stability dev --no-interaction
```

With `composer require ...` you can download new dependencies to your 
installation.

```
cd some-dir
composer require drupal/devel:8.*
```

The `composer create-project` command passes ownership of all files to the 
project that is created. You should create a new git repository, and commit 
all files not excluded by the .gitignore file.

## What does the template do?

When installing the given `composer.json` some tasks are taken care of:

* Drupal will be installed in the `web`-directory.
* Autoloader is implemented to use the generated composer autoloader in `vendor/autoload.php`,
  instead of the one provided by Drupal (`web/vendor/autoload.php`).
* Modules (packages of type `drupal-module`) will be placed in `web/modules/contrib/`
* Theme (packages of type `drupal-theme`) will be placed in `web/themes/contrib/`
* Profiles (packages of type `drupal-profile`) will be placed in `web/profiles/contrib/`
* Creates default writable versions of `settings.php` and `services.yml`.
* Creates `sites/default/files`-directory.
* Latest version of drush is installed locally for use at `vendor/bin/drush`.
* Latest version of DrupalConsole is installed locally for use at `vendor/bin/drupal`.

## Updating Drupal Core

This project will attempt to keep all of your Drupal Core files up-to-date; the 
project [drupal-composer/drupal-scaffold](https://github.com/drupal-composer/drupal-scaffold) 
is used to ensure that your scaffold files are updated every time drupal/core is 
updated. If you customize any of the "scaffolding" files (commonly .htaccess), 
you may need to merge conflicts if any of your modfied files are updated in a 
new release of Drupal core.

Follow the steps below to update your core files.

1. Run `composer update drupal/core`.
1. Run `git diff` to determine if any of the scaffolding files have changed. 
   Review the files for any changes and restore any customizations to 
  `.htaccess` or `robots.txt`.
1. Commit everything all together in a single commit, so `web` will remain in
   sync with the `core` when checking out branches or running `git bisect`.
1. In the event that there are non-trivial conflicts in step 2, you may wish 
   to perform these steps on a branch, and use `git merge` to combine the 
   updated core files with your customized files. This facilitates the use 
   of a [three-way merge tool such as kdiff3](http://www.gitshah.com/2010/12/how-to-setup-kdiff-as-diff-tool-for-git.html). This setup is not necessary if your changes are simple; 
   keeping all of your modifications at the beginning or end of the file is a 
   good strategy to keep merges easy.

## Generate composer.json from existing project

With using [the "Composer Generate" drush extension](https://www.drupal.org/project/composer_generate)
you can now generate a basic `composer.json` file from an existing project. Note
that the generated `composer.json` might differ from this project's file.


## FAQ

### Should I commit the contrib modules I download

Composer recommends **no**. They provide [argumentation against but also 
workrounds if a project decides to do it anyway](https://getcomposer.org/doc/faqs/should-i-commit-the-dependencies-in-my-vendor-directory.md).

### How can I apply patches to downloaded modules?

If you need to apply patches (depending on the project being modified, a pull 
request is often a better solution), you can do so with the 
[composer-patches](https://github.com/cweagans/composer-patches) plugin.

To add a patch to drupal module foobar insert the patches section in the extra 
section of composer.json:
```json
"extra": {
    "patches": {
        "drupal/foobar": {
            "Patch description": "URL to patch"
        }
    }
}
```
