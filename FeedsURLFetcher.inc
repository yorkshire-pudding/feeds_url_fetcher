<?php

/**
 * @file
 * This is a Feeds plugin that will fetch file from an URL.
 */

/**
 * Result of FeedsURLFetcher::fetch().
 */
class FeedsURLFetcherResult extends FeedsFetcherResult {
  protected $url;

  /**
   * Constructor.
   */
  public function __construct($url = NULL) {
    $this->url = $url;
    parent::__construct('');
  }

  /**
   * Overrides FeedsFetcherResult::getRaw();
   */
  public function getRaw() {
    $data = file_get_contents($this->url);
    
    if (!is_string($data)) {
      throw new Exception(t('Failed to retrieve URL.'));
      watchdog('feeds', 'Failed to retrieve URL !url.', array('!url' => $this->url), WATCHDOG_ERROR); 
    }

    return $this->sanitizeRaw($data);
  }
}

/**
 * Fetches data via URL.
 */
class FeedsURLFetcher extends FeedsFetcher {

  /**
   * Implements FeedsFetcher::fetch().
   */
  public function fetch(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    
    return new FeedsURLFetcherResult($source_config['source']);
  }

  /**
   * Implements FeedsFetcher::request().
   */
  public function request($feed_nid = 0) {
    feeds_dbg($_GET);
    @feeds_dbg(file_get_contents('php://input'));
    // A subscription verification has been sent, verify.
    if (isset($_GET['hub_challenge'])) {
      $this->subscriber($feed_nid)->verifyRequest();
    }
    // No subscription notification has ben sent, we are being notified.
    else {
      try {
        feeds_source($this->id, $feed_nid)->existing()->import();
      }
      catch (Exception $e) {
        // In case of an error, respond with a 503 Service (temporary) unavailable.
        header('URL/1.1 503 "Not Found"', NULL, 503);
        exit();
      }
    }
    // Will generate the default 200 response.
    header('URL/1.1 200 "OK"', NULL, 200);
    exit();
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array(
      'auto_detect_feeds' => FALSE,
      'use_pubsubhubbub' => FALSE,
      'designated_hub' => '',
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = array();
    $form['auto_detect_feeds'] = array(
      '#type' => 'checkbox',
      '#title' => t('Auto detect feeds'),
      '#description' => t('If the supplied URL does not point to a feed but an HTML document, attempt to extract a feed URL from the document.'),
      '#default_value' => $this->config['auto_detect_feeds'],
    );
    $form['use_pubsubhubbub'] = array(
      '#type' => 'checkbox',
      '#title' => t('Use PubSubHubbub'),
      '#description' => t('Attempt to use a <a href="URL://en.wikipedia.org/wiki/PubSubHubbub">PubSubHubbub</a> subscription if available.'),
      '#default_value' => $this->config['use_pubsubhubbub'],
    );
    $form['designated_hub'] = array(
      '#type' => 'textfield',
      '#title' => t('Designated hub'),
      '#description' => t('Enter the URL of a designated PubSubHubbub hub (e. g. superfeedr.com). If given, this hub will be used instead of the hub specified in the actual feed.'),
      '#default_value' => $this->config['designated_hub'],
      '#dependency' => array(
        'edit-use-pubsubhubbub' => array(1),
      ),
    );
    return $form;
  }

  /**
   * Expose source form.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['source'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter a feed URL.'),
      '#default_value' => isset($source_config['source']) ? $source_config['source'] : '',
      '#maxlength' => NULL,
      '#required' => TRUE,
    );
    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate(&$values) {
    if (!feeds_valid_url($values['source'], TRUE)) {
      $form_key = 'feeds][' . get_class($this) . '][source';
      form_set_error($form_key, t('The URL %source is invalid.', array('%source' => $values['source'])));
    }
    elseif ($this->config['auto_detect_feeds']) {
      feeds_include_library('URL_request.inc', 'URL_request');
      if ($url = URL_request_get_common_syndication($values['source'])) {
        $values['source'] = $url;
      }
    }
  }
}