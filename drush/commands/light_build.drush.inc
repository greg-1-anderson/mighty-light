<?php

/**
 * Implements hook_drush_command().
 */
function light_build_drush_command() {
  $items = array();

  $items['light-build'] = array(
    'description' => "Drush command light-build.",
    // Describe the arguments for this command.  Delete
    // this seciton if command takes no arguments.
    'arguments' => array(
      'arg' => 'Description of command argument.',
    ),
    // List all options used for this command. Only options that
    // are listed here (or that are global Drush options) may
    // be specified on the commnadline; all others raise an error.
    'options' => array(
      'option-name' => array(
        'description' => 'Description of option.',
        'example-value' => 'Example values for option, if required.',
      ),
    ),
    // Give one or more example commandline usages for this command
    'examples' => array(
      'drush light-build --option-name arg' => 'Do something.',
    ),
    // Delete the 'outputformat' record if command does not
    // produce any output.
    'outputformat' => array(
      'default' => 'table',
      'pipe-format' => 'var_export',
      'field-labels' => array(
        'lang' => 'Language',
        'msg' => 'Message',
      ),
      'fields-default' => array('lang', 'msg'),
      'output-data-type' => 'format-table',
    ),
    'aliases' => array('shortcut'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all. Remove for full bootstrap of Drupal site.
  );

  // Commandfiles may also add topics.  These will appear in
  // the list of topics when `drush topic` is executed.
  $topic_file = dirname(__FILE__) . '/light-build-topic.txt';
  if (file_exists($topic_file)) {
    $items['docs-light-build'] = array(
      'description' => 'Description of command topic.',
      'hidden' => TRUE,
      'topic' => TRUE,
      'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
      'callback' => 'drush_print_file',
      'callback arguments' => array($topic_file),
    );
  }

  return $items;
}

/**
 * Implements hook_drush_help().
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function light_build_drush_help($section) {
  switch ($section) {
    case 'drush:light-build':
      return dt("Brief help for Drush command light-build.");
    // The 'title' meta item is used to name a group of
    // commands in `drush help`.  If a title is not defined,
    // the default is "All commands in ___", with the
    // specific name of the commandfile (e.g. light_build).
    // Command files with less than four commands will
    // be placed in the "Other commands" section, _unless_
    // they define a title.  It is therefore preferable
    // to not define a title unless the file defines a lot
    // of commands.
    case 'meta:light_build:title':
      return dt("light_build commands");
    // The 'summary' meta item is displayed in `drush help --filter`,
    // and is used to give a general idea what the commands in this
    // command file do, and what they have in common.
    case 'meta:light_build:summary':
      return dt("Summary of all commands in this command group.");
  }
}

/**
 * Implementation of drush_hook_COMMAND().
 *
 * This script receives requests initiated from a CI test server.
 * GitHub should be set up to send a request to a Patheon multidev
 * environment named 'build' every time something is pushed to the
 * GitHub repository's master branch.
 *
 * This script will then in turn pull the code and, if composer.lock
 * has been properly committed, will run 'composer install' and commit
 * the result back to the 'fat' repository.  From here, it can be
 * deployed to test and live as usual.
 *
 *    Github             CI              Multidev           Test
 *      |                |                   |                  |
 *      |--- webhook --->|                   |                  |
 *      |                |                   |                  |
 *      |                |---- drush cmd --->|                  |
 *      |                |                   |                  |
 *      | ---------------------- pull -------|                  |
 *      | +->            |                   |                  |
 *      |                |                   |-------+          |
 *      |                |                   |       |          |
 *      |                |                   |    composer      |
 *      |                |                   |    install       |
 *      |                |                   |       |          |
 *      |                |                   |<------+          |
 *      |                |                   |                  |
 *      |                |                   |----- deploy ---->|
 *      |                |                   |                  |
 *
 * The multidev environment remains in SFTP mode so that it can
 * pull from the remote repository and run `composer install`.
 *
 * By default, the 'build' multidev environment builds everything
 * that is pushed from the 'master' branch; other multidev environments
 * assume that the remote branch name is the same as the local branch
 * name (which is the same as the multidev environment name).
 *
 * Test and live should never be targets.
 */
function drush_light_build() {

  $repositoryRoot = dirname(dirname(__DIR__)); // or get Drupal root or ask git
  $bindingDir = dirname($repositoryRoot); // or $_SERVER['HOME']
  $composerRoot = "$repositoryRoot/private";

  include_once "$repositoryRoot/private/scripts/pantheon/lean-repo-utils.php";

  // Determine which local branch we are going to merge into.
  $localBranch = $_ENV['PANTHEON_ENVIRONMENT'];
  if ($localBranch == 'dev') {
    $localBranch = 'master';
  }
  // Silently do nothing if this is 'test' or 'live'.
  if (in_array($localBranch, array('test', 'live'))) {
    return drush_set_error('LIGHT_BUILD_INVALID_TARGET', dt("The !env environment cannot be the target of a build", array('!env' => $localBranch)));
  }

  // Fetch our secret data / parameters
  $secrets = pantheon_get_secrets($bindingDir, ['lean-repo'], ['lean-gh-token' => '', 'lean-remote-branch' => '', 'lean-require-github' => false, 'lean-start-fresh' => false]);

  $githubUrl = $secrets['lean-repo']; // e.g. https://github.com/joshkoenig/lean-and-mean.git';
  $githubToken = $secrets['lean-gh-token'];
  $remoteBranch = $secrets['lean-remote-branch'];
  if (empty($githubUrl)) {
    return drush_set_error('LIGHT_BUILD_NO_GITHUB_URL', dt('Secrets file does not contain a Github URL.'));
  }
  if (!empty($githubToken)) {
    $githubUrl = str_replace('https://',
                             'https://'. $githubToken . ':x-oauth-basic@',
                             $githubUrl);
  }
  /*
  else {
    return drush_set_error('LIGHT_BUILD_NO_GITHUB_TOKEN', dt('No Github Token found'));
  }
  */

  // Blow away the lean upstream if 'lean-start-fresh' is set (repair)
  if ($secrets['lean-start-fresh']) {
    exec("git branch -D _lean_upstream", $deleteOutput, $status);
  }

  // Figure out what the remote branch should be.  This is usually going
  // to be the environment name (or 'master') for dev, but the 'build'
  // multidev also builds from master, and you may target any remote
  // branch you wish by setting an appropriate entry in the secrets file.
  if (empty($remoteBranch)) {
    $remoteBranch = $localBranch == 'build' ? 'master' : $localBranch;
  }

  chdir($repositoryRoot);
  $result = light_build_process_github_webhook($githubUrl, $remoteBranch);
  if (!$result) {
    return $result;
  }

  // If composer.lock has been committed, then run `composer install`
  // Note that `composer update` requires too much memory to run here,
  // and `composer install` without a lock file behaves equivalently.
  if (file_exists("$composerRoot/composer.lock")) {
    chdir($composerRoot);
    exec('composer install', $composerInstallOutput, $status);
    if ($status) {
      return drush_set_error('LIGHT_BUILD_INSTALL_FAILED', dt('Composer install failed. Output: !output', array('!output' => "\n\n" . implode("\n", $composerInstallOutput))));
    }
    else {
      drush_print("Ran 'composer install':\n");
      drush_print(implode("\n", $composerInstallOutput));
    }
    $result = light_build_commit_build_results($repositoryRoot);
    if (!$result) {
      return $result;
    }
  }

  // Push merged lean changes up to Pantheon's internal repo.
  // This will trigger a sync_code event.
  exec("git push origin $localBranch", $gitPushOutput, $status);
  print_r($gitPushOutput);
}

function light_build_commit_build_results($repositoryRoot) {
  // Prepare to commit build results.
  // The composer-generated files -could- be added with --force,
  // but we still need to know which files to add.  Therefore,
  // we will instead modify the .gitignore file to permit the
  // addition of the generated files, and then put it back
  // again when we are done.  This allows us to use the .gitignore
  // file to list which directories contain build results.
  // There is a marker in the .gitignore that separates the
  // build results entries, which should be added to the Pantheon
  // repository, but should not be added to the lean repository,
  // from all of the other .gitignore entries which should never be
  // added to either repository.  Everything above the marker are
  // build results.  We get rid of these entries temporarily when
  // adding to the Pantheon repository.  We also add '.gitignore' to
  // the top of the file, so that we do not add the .gitignore file itself
  // to the Pantheon reposiotry during this step.
  $gitignoreFile = "$repositoryRoot/.gitignore";
  $gitignoreContents = file_get_contents($gitignoreFile);
  $markerPos = strpos($gitignoreContents, "### Persistent .gitignore entries:");
  if ($markerPos !== FALSE) {
    $reducedContents = ".gitignore\n\n" . substr($gitignoreContents, $markerPos);
    file_put_contents($gitignoreFile, $reducedContents);
    // Commit build results
    $gitCommitStatus = 0;
    exec('git add -A .', $gitAddOutput, $gitAddStatus);
    if (!$gitAddStatus) {
      exec('git commit -m "Commit build results."', $gitCommitOutput, $gitCommitStatus);
    }
    // restore gitignore. We could also run `git checkout -- .gitignore`
    file_put_contents($gitignoreFile, $gitignoreContents);
    if ($gitAddStatus) {
      return drush_set_error('LIGHT_BUILD_ADD_FAILED', dt('Git add failed.  Output: !output', array('!output' => "\n\n" . implode("\n", $gitAddOutput))));
    }
    if ($gitCommitStatus) {
      return drush_set_error('Git commit failed. Output: !output', array('!output' => "\n\n" . implode("\n", $gitCommitOutput)));
    }
  }
  return TRUE;
}


/**
 * Process a webhook received from GitHub.
 */
function light_build_process_github_webhook($remoteUrl, $remoteBranch) {
  try {
    // Fetch the master branch from the remote URL.  Put it in the
    // '_lean_upstream' branch in the local repository, creating it
    // if necessary.
    exec("git fetch $remoteUrl $remoteBranch:_lean_upstream", $gitFetchOutput, $status);
    if ($status !== 0) {
      return drush_set_error('LIGHT_BUILD_FETCH_FAILED', dt('Error fetching from GitHub - !status. Output: !output', array('!output' => "\n\n" . implode("\n", $gitFetchOutput))));
    }
    // TODO: Pull commit message from head commit of _lean_upstream
    $commit_message = 'Simulated test.';
    // Merge the _lean_upstream branch into the current Multidev
    // branch.  Use -Xtheirs, so that any conflict is satisfied in
    // favor of the code coming in from the remote repository.
    exec('git merge -s recursive -Xtheirs _lean_upstream -m "From upstream: '. $commit_message .'"', $gitMergeOutput, $status);
    if ($status == 128) {
      return drush_set_error('LIGHT_BUILD_UNCOMMITTED_CHANGES', dt("Uncommitted changes present - Merge blocked.  Output: !output", array('!output' => "\n\n" . implode("\n", $gitMergeOutput))));
    }
    elseif ($status !== 0) {
      return drush_set_error('LIGHT_BUILD_MERGE_ERROR', dt("Merge error.  Output: !output", array('!output' => "\n\n" . implode("\n", $gitMergeOutput))));
    }
  }
  catch (Exception $e) {
    // Try and emit an error message to the dashboard if there was a fail.
    return drush_set_error('LIGHT_BUILD_EXCEPTION', $e->getMessage());
  }
}

