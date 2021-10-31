<?php

/**
 * @file
 * Class for loading, modifying, and executing a layout.
 */
class MiniLayout extends Layout {
  /**
   * The admin label of the mini layout.
   *
   * @var string
   */
  var $info = '';

  /**
   * The block delta of the mini layout.
   *
   * @var string
   */
  var $delta = '';

  /**
   * The block delta of the mini layout.
   *
   * @var string
   */
  var $module = 'mini_layouts';

  /**
   * An array of LayoutContext instances used by this menu item.
   *
   * @var LayoutContext[]
   */
  private $contexts = array();

  /**
   * The internal path of a layout.
   *
   * The layout path must match the provided menu item path. Set and get this
   * variable's value with Layout::getPath() and Layout::setPath().
   *
   * @var string
   */
  private $path;


  /**
   * Constructor for a Layout class.
   *
   * @param array $config
   *   An array of configuration data.
   */
  function __construct(array $config = array()) {
    if (empty($this->path)) {
      $this->path = uniqid();
    }
    parent::__construct($config);
    // $this->path = $config['delta'];
    // Ensure contexts is always set to an array.
    if (empty($this->contexts)) {
      $this->contexts = array();
    }
  }

  /**
   * Save a layout to config.
   */
  function save() {
    $this->contexts = $this->getContexts();
          dpm($this->contexts);
  if ($this->storage === LAYOUT_STORAGE_DEFAULT) {
      $this->storage = LAYOUT_STORAGE_OVERRIDE;
    }
    $is_new = !empty($this->is_new);

    // Don't save path based contexts as these are generated on the fly.
    foreach ($this->contexts as $key => $context) {
        dpm('aha');
      if (!$context->storage) {
        unset($this->contexts[$key]);
      }
    }

    // Allow modules to act on layout data before saving.
    foreach (module_implements('layout_presave') as $module) {
      $function = $module . '_layout_presave';
      $function($this);
    }

    $data = array(
      'path' => $this->path,
      'name' => $this->name,
      'info' => $this->info,
      'title' => $this->title,
      'description' => $this->description,
      'renderer_name' => $this->renderer_name,
      'module' => $this->module,
      'weight' => $this->weight,
      'storage' => $this->storage,
      'layout_template' => $this->layout_template,
      'disabled' => $this->disabled,
      'settings' => $this->settings,
      'positions' => $this->positions,
      'contexts' => $this->contexts,
      'relationships' => $this->relationships,
    );

    // Basic validation to prevent data corruption.
    if (empty($this->name)) {
      throw new LayoutSaveException(t('The layout must have a name specified to save.'));
    }

    // Convert all stored conditions to arrays.
    // @todo: Save per-layout contexts and relationships.
    $sub_parts = array(
      'content',
      'conditions',
      'contexts',
      'relationships',
    );
    foreach ($sub_parts as $config_type) {
      foreach ($this->$config_type as $config_type_key => $config_type_data) {
        unset($config_type_data->is_new);
        $data[$config_type][$config_type_key] = array(
          'plugin' => $config_type_data->plugin,
          'data' => $config_type_data->toArray(),
        );
      }
    }

    if (isset($this->original_name) && $this->original_name != $this->name) {
      config('layout.layout.' . $this->original_name)->delete();
    }
    config('layout.mini_layouts.' . $this->name)
      ->setData($data)
      ->save();

    // Parse each BlockText block for file IDs.
    $fids = array();
    foreach ($this->content as $content) {
      if (is_a($content, 'BlockText')) {
        $block_content = $content->settings['content'];
        $block_fids = filter_parse_file_fids($block_content);
        if (isset($block_fids)) {
          $fids = array_merge($fids, $block_fids);
        }
      }
    }
    $files = file_load_multiple($fids);
    foreach ($files as $fid => $file) {
      if ((int) ($file && $file->status) !== FILE_STATUS_PERMANENT) {
        // This makes the file "self-referencing", so it will never be deleted.
        // File usages are not removed for text blocks currently.
        // See https://github.com/backdrop/backdrop-issues/issues/2137.
        file_usage_add($file, 'file', 'file', $file->fid);
      }
    }

    layout_reset_caches();
  }

  /**
   * Return a form for configuring this layout's settings.
   */
  function form(&$form, &$form_state) {}

  /**
   * Delete this layout.
   */
  function delete() {
    config('layout.mini_layouts.' . $this->name)->delete();
    $this->invokeHook('delete');
    layout_reset_caches();
    cache()->delete('layout:mini_layouts:config');
    backdrop_static_reset('mini_layouts_block_info');
  }

}
