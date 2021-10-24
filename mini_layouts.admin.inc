<?php
/**
 * @file
 * Admin page callbacks for the Block module.
 */

/**
 * Page callback; Display a list of all custom blocks.
 */
function mini_layouts_admin_list() {
  $custom_blocks_info = mini_layouts_block_info();

  $header = array(
    t('Block'),
    array('data' => t('Description'), 'class' => array('priority-low')),
    t('Operations'),
  );

  $rows = array();
  foreach ($custom_blocks_info as $delta => $block_info) {
    $block = mini_layouts_custom_block_load($delta);
    $row = array();
    $row[] = theme('label_machine_name__block', array(
      'label' => $block_info['info'],
      'machine_name' => $delta,
    ));
    $row[] = filter_xss($block_info['description']);
    $links = array();
    $links['configure'] = array(
      'title' => t('Configure'),
      'href' => 'admin/structure/mini-layouts/manage/' . $delta . '/configure',
    );
    $links['delete'] = array(
      'title' => t('Delete'),
      'href' => 'admin/structure/mini-layouts/manage/' . $delta . '/delete',
    );
    $links['configure_blocks'] = array(
      'title' => t('Configure layout template'),
      'href' => 'admin/structure/mini-layouts/manage/' . $delta . '/configure-layout-blocks',
    );
    if (user_access('synchronize configuration')) {
      $links['export'] = array(
        'title' => t('Export'),
        'href' => 'admin/config/development/configuration/single/export',
        'query' => array(
          'group' => 'Custom Blocks',
          'name' => 'block.mini_layouts.' . $delta,
        ),
      );
    }
    $operations = array(
      '#type' => 'operations',
      '#links' => $links,
    );
    $row[] = array('data' => $operations);

    $rows[] = $row;
  }

  return array(
    '#theme' => 'table__mini_layouts_admin_list',
    '#rows' => $rows,
    '#header' => $header,
    '#empty' => t('No custom blocks have been created yet.'),
  );
}

/**
 * Form constructor for the block configuration form.
 *
 * Also used by mini_layouts_add_block_form() for adding a new custom block.
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 * @param $delta
 *   Unique ID of the block within the context of $module.
 *
 * @see mini_layouts_menu()
 * @see mini_layouts_admin_configure_validate()
 * @see mini_layouts_admin_configure_submit()
 * @ingroup forms
 */
function mini_layouts_admin_configure($form, &$form_state, $delta = NULL) {
  if ($delta) {
    $langcode = isset($form['langcode']) ? $form['langcode']['#value'] : NULL;
    $custom_block = mini_layouts_custom_block_load($delta, $langcode);
    if ($langcode) {
      $custom_block['langcode'] = $langcode;
    }

    if (!$custom_block) {
      backdrop_not_found();
      exit();
    }
  }
  else {
    $custom_block = array(
      'info' => '',
      'title' => '',
      'body' => array('value' => '', 'format' => NULL),
      'layout_template' => 'boxton',
    );
  }
    dpm($custom_block);
  if ($custom_block['info']) {
    backdrop_set_title(t("'%name' block", array('%name' => $custom_block['info'])), PASS_THROUGH);
  }
  else {
    backdrop_set_title(t('Add custom block'));
  }

  // Call our own block configuration form (also used by Layout module).
  $form += mini_layouts_custom_block_form($custom_block, TRUE);

  // Prepare cancel link.
  if (isset($_GET['destination'])) {
    $path = $_GET['destination'];
  }
  elseif (isset($_SERVER['HTTP_REFERER'])) {
    $path = $_SERVER['HTTP_REFERER'];
  }
  elseif (user_access('administer blocks')) {
    $path = 'admin/structure/mini-layouts';
  }
  else {
    $path = '<front>';
  }
  $options = backdrop_parse_url($path);
  $options['attributes']['class'][] = 'form-cancel';
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save block'),
  );
  if ($delta) {
    $form['actions']['delete'] = array(
      '#type' => 'link',
      '#title' => t('Delete'),
      '#href' => 'admin/structure/mini-layouts/manage/' . $delta . '/delete',
      '#attributes' => array('class' => array('button', 'button-secondary', 'form-delete')),
    );
  }
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => $options['path'],
    '#options' => $options,
    '#weight' => 1,
  );

  return $form;
}

/**
 * Form validation handler for mini_layouts_admin_configure().
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @see mini_layouts_admin_configure()
 * @see mini_layouts_admin_configure_submit()
 */
function mini_layouts_admin_configure_validate($form, &$form_state) {
  $blocks = mini_layouts_block_info();
  foreach ($blocks as $delta => $block) {
    if ($delta != $form_state['values']['delta'] && $block['info'] == $form_state['values']['info']) {
      form_set_error('info', t('Ensure that each block description is unique.'));
      return;
    }
  }
}

/**
 * Form submission handler for mini_layouts_admin_configure().
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @see mini_layouts_admin_configure()
 * @see mini_layouts_admin_configure_validate()
 */
function mini_layouts_admin_configure_submit($form, &$form_state) {
  mini_layouts_custom_block_save($form_state['values'], $form_state['values']['delta']);
  backdrop_set_message(t('The block configuration has been saved.'));
  // $form_state['redirect'] = 'admin/structure/mini-layouts';
}

/**
 * Form constructor for the add block form.
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @see mini_layouts_menu()
 * @see mini_layouts_add_block_form_validate()
 * @see mini_layouts_add_block_form_submit()
 * @ingroup forms
 */
function mini_layouts_add_block_form($form, &$form_state) {
  return mini_layouts_admin_configure($form, $form_state);
}

/**
 * Form validation handler for mini_layouts_add_mini_layouts_form().
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @see mini_layouts_add_mini_layouts_form()
 * @see mini_layouts_add_block_form_submit()
 */
function mini_layouts_add_block_form_validate($form, &$form_state) {
  return mini_layouts_admin_configure_validate($form, $form_state);
}

/**
 * Form submission handler for mini_layouts_add_block_form().
 *
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * Saves the new custom block.
 *
 * @see mini_layouts_add_block_form()
 * @see mini_layouts_add_block_form_validate()
 */
function mini_layouts_add_block_form_submit($form, &$form_state) {
  mini_layouts_custom_block_save($form_state['values'], $form_state['values']['delta']);
  backdrop_set_message(t('The block has been created.'));
  $form_state['redirect'] = 'admin/structure/mini-layouts';
}

/**
 * Form constructor for the custom block deletion form.
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 * @param $delta
 *   The unique ID of the block within the context of $module.
 *
 * @see mini_layouts_menu()
 * @see mini_layouts_custom_block_delete_submit()
 */
function mini_layouts_custom_block_delete($form, &$form_state, $delta) {
  $custom_block = mini_layouts_custom_block_load($delta);
  $form['info'] = array('#type' => 'hidden', '#value' => $custom_block['info']);
  $form['delta'] = array('#type' => 'hidden', '#value' => $delta);

  return confirm_form($form, t('Are you sure you want to delete the block %name?', array('%name' => $custom_block['info'])), 'admin/structure/mini-layouts', '', t('Delete'), t('Cancel'));
}

/**
 * Form submission handler for mini_layouts_custom_block_delete().
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @see mini_layouts_custom_block_delete()
 */
function mini_layouts_custom_block_delete_submit($form, &$form_state) {
  config('block.mini_layouts.' . $form_state['values']['delta'])->delete();
  backdrop_set_message(t('The block %name has been removed.', array('%name' => $form_state['values']['info'])));
  $form_state['redirect'] = 'admin/structure/mini-layouts';
}
