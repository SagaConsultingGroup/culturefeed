<?php

function culturefeed_user_create($cf_uid) {
  $url = parse_url($GLOBALS['base_url']);
  $account = user_save(
    drupal_anonymous_user(),
    array(
      'name' => $cf_uid,
      'pass' => user_password(),
      'mail' => $cf_uid . '@uitid.be',
      'roles' => array(),
      'status' => 1,
      'timezone' => variable_get('date_default_timezone', date_default_timezone_get()),
    )
  );

  return $account;
}

function culturefeed_user_get($cf_uid, $token = NULL, $secret = NULL, $application_key = NULL) {
  $account = NULL;
  
  if (!$application_key) {
    $application_key = variable_get('culturefeed_api_application_key', '');
  }

  $user_exists = db_query("SELECT * FROM {culturefeed_user} cfu WHERE cfu.cf_uid = :cf_uid", array(':cf_uid' => $cf_uid))->fetchObject();
  
  if (!$user_exists) {
    $account = culturefeed_user_create($cf_uid);
    db_insert('culturefeed_user')
      ->fields(array(
        'uid' => $account->uid,
        'cf_uid' => $cf_uid,
        ))
      ->execute();
  }
  else {
    $account = user_load($user_exists->uid);
  }
  
  if ($token) {
    $token_exists = db_query("SELECT * FROM {culturefeed_token} cft WHERE cft.cf_uid = :cf_uid AND cft.application_key = :application_key", array(':cf_uid' => $cf_uid, ':application_key' => $application_key))->fetchObject();
    
    db_delete('culturefeed_token')
      ->condition('cf_uid', $cf_uid)
      ->condition('application_key', $application_key)
      ->execute();
    
    db_insert('culturefeed_token')
      ->fields(array(
        'cf_uid' => $cf_uid,
        'token' => $token,
        'secret' => $secret,
        'application_key' => $application_key,
        ))
      ->execute();
  }

  return $account;
}

function culturefeed_get_events_for_activities($activities) {
  $events = array();

  foreach ($activities as $activity) {
    if ($activity->contentType == CultureFeed_Activity::CONTENT_TYPE_EVENT) {
      $events[$activity->nodeId] = NULL;
    }
  }

  $cdbids = array_unique(array_keys($events));

  return culturefeed_get_events_for_cdbids($cdbids);
}

function culturefeed_get_events_for_cdbids($cdbids) {
  $results = cnapi_get_events(array('cdbid' => implode(';', $cdbids), 'pagelength' => count($cdbids)));

  foreach ($results['data'] as $result) {
    $events[$result['cdbid']] = $result;
  }

  return $events;
}

function culturefeed_log($type, $exception) {
  watchdog($type, '[%error]: !message in %file on line %line.', array('%error' => $exception->getCode(), '!message' => $exception->getMessage(), '%file' => $exception->getFile(), '%line' => $exception->getLine()), WATCHDOG_ERROR);
}