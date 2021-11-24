<?php
/**
 * @file
 * Admin page callbacks for the Block module.
 */

/**
 * Page callback; Display a list of all mini layout blocks.
 */
function mini_layouts_admin_list() {
  $mini_layouts_block_info = mini_layouts_block_info();

  $header = array(
    t('Block'),
    array('data' => t('Description'), 'class' => array('priority-low')),
    t('Operations'),
  );

  $rows = array();
  foreach ($mini_layouts_block_info as $name => $block_info) {
    $row = array();
    $row[] = theme('label_machine_name__block', array(
      'label' => $block_info['info'],
      'machine_name' => $name,
    ));
    $row[] = filter_xss($block_info['description']);
    $links = array();
    $links['configure_blocks'] = array(
      'title' => t('Manage blocks'),
      'href' => 'admin/structure/mini-layouts/manage/' . $name . '/configure-layout-blocks',
    );
    $links['configure'] = array(
      'title' => t('Configure'),
      'href' => 'admin/structure/mini-layouts/manage/' . $name . '/configure',
    );
    $links['delete'] = array(
      'title' => t('Delete'),
      'href' => 'admin/structure/mini-layouts/manage/' . $name . '/delete',
    );
    if (user_access('synchronize configuration')) {
      $links['export'] = array(
        'title' => t('Export'),
        'href' => 'admin/config/development/configuration/single/export',
        'query' => array(
          'group' => 'Mini layouts blocks',
          'name' => 'layout.mini_layouts.' . $name,
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
    '#empty' => t('No mini layouts blocks have been created yet.'),
  );
}


/**
 * Form constructor for the block configuration form.
 *
 * Also used by mini_layouts_add_block_form() for adding a new mini layouts block.
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 * @param $name
 *   Unique ID of the block within the context of $module.
 *
 * @see mini_layouts_menu()
 * @see mini_layouts_admin_configure_validate()
 * @see mini_layouts_admin_configure_submit()
 * @ingroup forms
 */
function mini_layouts_admin_configure($form, &$form_state, $name = NULL) {
  $form['#attached']['js'][] = backdrop_get_path('module', 'layout') . '/js/layout.admin.js';
  $form['#attached']['css'][] = backdrop_get_path('module', 'mini_layouts') . '/mini_layouts.admin.css';

  if ($name) {
    $mini_layout = layout_tempstore_load($name);

    if (!$mini_layout) {
      $mini_layout = mini_layouts_load($name);
    }
    elseif (!$mini_layout) {
      backdrop_not_found();
      exit();
    }
  }
  else {
    $config = array(
      'info' => '',
      'title' => '',
      'body' => array('value' => '', 'format' => NULL),
      'layout_template' => 'boxton',
    );
    $mini_layout = new MiniLayout($config);
  }
  $form_state['mini_layout'] = $mini_layout;

  if ($mini_layout->info) {
    backdrop_set_title(t("'%name' block", array('%name' => $mini_layout->info)), PASS_THROUGH);
  }
  else {
    backdrop_set_title(t('Add mini layouts block'));
  }

  $form['info'] = array(
    '#type' => 'textfield',
    '#title' => t('Admin label'),
    '#default_value' => $mini_layout->info,
    '#maxlength' => 64,
    '#description' => t('This text is used only in administrative interfaces. It will not be shown to site visitors.'),
    '#required' => TRUE,
    '#id' => 'block-info',
  );
  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => $mini_layout->name,
    '#maxlength' => 128,
    '#machine_name' => array(
      'exists' => 'mini_layouts_load',
      'source' => array('info'),
    ),
    '#description' => t('A unique machine-readable name for this Block. It must only contain lowercase letters, numbers, and underscores.'),
    '#disabled' => isset($mini_layout->name),
  );
  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Admin description'),
    '#default_value' => $mini_layout->description,
    '#maxlength' => 128,
    '#description' => t('This text is used only in administrative interfaces. It will not be shown to site visitors.<br />Allowed HTML tags: @tags', array('@tags' => _filter_xss_display_allowed_tags())),
    '#id' => 'block-description',
  );
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Display title'),
    '#default_value' => $mini_layout->title,
    '#maxlength' => 255,
    '#description' => t('The title of the block as shown to the user. This will affect all places where this block is used.'),
  );
  // todo get rid of this of course.
  $form['body'] = array(
    '#type' => 'value',
    '#value' => TRUE,
  );

  $form['layout_template'] = array(
    '#title' => t('Layout template'),
    '#type' => 'radios',
    '#default_value' => $mini_layout->layout_template,
    '#options' => array(),
    '#wrapper_attributes' => array('class' => array('clearfix', 'layout-options')),
    '#required' => TRUE,
  );

  // Get the list of layout template options. The list needs to be rebuilt (see
  // https://github.com/backdrop/backdrop-issues/issues/984)
  $all_template_info = layout_get_layout_template_info(NULL, TRUE);

  $excluded = config_get('layout.settings', 'excluded_templates');
  foreach ($all_template_info as $template_name => $template_info) {
    if (!in_array($template_name, $excluded)) {
      $form['layout_template']['#options'][$template_name] = theme('layout_template_option', array('template_info' => $template_info));
    }
  }

  // Get all contexts except those provided by relationships.
  $contexts = $mini_layout->getContexts(LayoutContext::USAGE_TYPE_ALL ^ LayoutContext::USAGE_TYPE_RELATIONSHIP);
  $built_in_contexts = isset($contexts['overrides_path']) ? 2 : 1;
  $form['context_wrapper'] = array(
    '#title' => t('Contexts'),
    '#type' => 'item',
    '#id' => 'context-wrapper',
    '#description' => t('(Optional) Contexts define what blocks are available within this layout. Most placeholders in the path will be associated with a context automatically.'),
    // Current user context is always present.
  );
  $form['context_wrapper']['#access'] = !$mini_layout->isDefault();
  $form['context_wrapper']['#prefix'] = '<div id="layout-contexts">';
  $form['context_wrapper']['#suffix'] = '</div>';
  $form['context_wrapper']['context'] = array(
    '#type' => 'container',
    '#parents' => array('context'),
  );

  $form['context_wrapper']['context']['links'] = array(
    '#theme' => 'layout_action_links',
  );
  $form['context_wrapper']['context']['links']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add context'),
    '#attributes' => array('class' => array('layout-link-button', 'layout-access-add')),
    '#validate' => array(),
    '#submit' => array(
      'layout_settings_form_update_layout',
      'mini_layouts_settings_form_context_add',
    ),
    '#ajax' => array(
      'callback' => 'layout_ajax_form_open_dialog',
    ),
  );

  // Get contexts from relationships
  $relevant_relationships = layout_relationships_get_relevant_info($contexts);
  if (!empty($relevant_relationships)) {
    $form['context_wrapper']['context']['links']['add_relationship'] = array(
      '#type' => 'submit',
      '#name' => 'relationship_add_button',
      '#value' => t('Add relationship'),
      '#attributes' => array('class' => array('layout-link-button', 'layout-access-add')),
      '#validate' => array(),
      '#submit' => array(
        'layout_settings_form_update_layout',
        'mini_layouts_settings_form_context_add',
      ),
      '#ajax' => array(
        'callback' => 'layout_ajax_form_open_dialog',
      ),
    );
  }

  $all_context_info = _layout_get_all_info('layout_context');
  $context_options = array();
  foreach ($all_context_info as $plugin_name => $context_info) {
    if (empty($context_info['hidden'])) {
      $context_options[$plugin_name] = $context_info['title'];
    }
  }

  $form['context_wrapper']['context']['required'] = array(
    '#theme' => 'layout_settings_context_table',
    '#layout_path' => $mini_layout->getPath(),
    '#access' => count($contexts) > $built_in_contexts,
  );

dpm($contexts);
  foreach ($contexts as $context_key => $layout_context) {
    if ($layout_context->usageType !== LayoutContext::USAGE_TYPE_SYSTEM) {
      // Contexts that are locked to a particular position (such as node/%).
      if ($layout_context->position && $layout_context->required) {
        $form['context_wrapper']['context']['required'][$context_key]['summary'] = array(
          '#markup' => $layout_context->getAdminSummary($mini_layout->getPath()),
        );
        if ($layout_context->locked) {
          $form['context_wrapper']['context']['required'][$context_key]['plugin'] = array(
            '#markup' => check_plain($all_context_info[$layout_context->plugin]['title']),
          );
        }
        else{
          $form['context_wrapper']['context']['required'][$context_key]['plugin'] = array(
            '#type' => 'select',
            '#options' => $context_options,
            '#default_value' => $layout_context->plugin,
          );
        }
      }
      // Custom contexts and required contexts that do not need position (such
      // as admin/dashboard).
      else {
        $form['context_wrapper']['context']['required'][$context_key]['summary'] = array(
          '#markup' => $layout_context->getAdminSummary($mini_layout->getPath()),
        );
        $form['context_wrapper']['context']['required'][$context_key]['plugin'] = array(
          '#markup' => check_plain($all_context_info[$layout_context->plugin]['title']),
        );
      }
      $form['context_wrapper']['context']['required'][$context_key]['operations'] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('layout-operations')),
      );
      $form['context_wrapper']['context']['required'][$context_key]['operations']['remove'] = array(
        '#type' => 'submit',
        '#value' => t('Remove'),
        '#attributes' => array('class' => array('layout-link-button', 'layout-context-remove')),
        '#validate' => array(),
        '#submit' => array(
          'mini_layouts_settings_form_context_remove',
          'layout_settings_form_update_layout',
        ),
        '#ajax' => array(
          'callback' => 'layout_ajax_form_update',
        ),
        '#name' => 'conditions_' . $context_key . '_remove',
      );
      $form['context_wrapper']['context']['required'][$context_key]['operations']['configure'] = array(
        '#type' => 'submit',
        '#value' => t('Configure'),
        '#attributes' => array('class' => array('layout-link-button', 'layout-context-configure')),
        '#validate' => array(
          //'layout_settings_form_validate',
        ),
        '#submit' => array(
          'mini_layouts_settings_form_context_edit',
        ),
        '#ajax' => array(
          'callback' => 'layout_ajax_form_open_dialog',
        ),
        '#name' => 'conditions_' . $context_key . '_configure',
      );
    }
  }

  $all_relationship_info = _layout_get_all_info('layout_relationship');
  foreach ($mini_layout->relationships as $relationship_key => $relationship) {
    $form['context_wrapper']['context']['required'][$relationship_key]['summary'] = array(
      '#markup' => $relationship->getAdminSummary(),
    );
    $form['context_wrapper']['context']['required'][$relationship_key]['plugin'] = array(
      '#markup' => isset($all_relationship_info[$relationship->plugin]['title']) ?
        check_plain($all_relationship_info[$relationship->plugin]['title']) :
        t('Broken'),
    );
    $form['context_wrapper']['context']['required'][$relationship_key]['operations'] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('layout-operations')),
    );
    $form['context_wrapper']['context']['required'][$relationship_key]['operations']['remove'] = array(
      '#type' => 'submit',
      '#value' => t('Remove'),
      '#attributes' => array('class' => array('layout-link-button', 'layout-context-remove')),
      '#validate' => array(),
      '#submit' => array(
        'mini_layouts_settings_form_context_relationship_remove',
        'layout_settings_form_update_layout',
      ),
      '#ajax' => array(
        'callback' => 'layout_ajax_form_update',
      ),
      '#name' => 'conditions_' . $relationship_key . '_remove',
    );
    $form['context_wrapper']['context']['required'][$relationship_key]['operations']['configure'] = array(
      '#type' => 'submit',
      '#value' => t('Configure'),
      '#attributes' => array('class' => array('layout-link-button', 'layout-context-configure')),
      '#validate' => array(
        //'layout_settings_form_validate',
      ),
      '#submit' => array(
        'mini_layouts_settings_form_context_relationship_edit',
      ),
      '#ajax' => array(
        'callback' => 'layout_ajax_form_open_dialog',
      ),
      '#name' => 'conditions_' . $relationship_key . '_configure',
    );
  }

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
    '#value' => t('Save mini layout'),
  );
  if ($name) {
    $form['actions']['delete'] = array(
      '#type' => 'link',
      '#title' => t('Delete'),
      '#href' => 'admin/structure/mini-layouts/manage/' . $name . '/delete',
      '#attributes' => array('class' => array('button', 'button-secondary', 'form-delete')),
    );
  }
  $form['actions']['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#limit_validation_errors' => array(),
    '#submit' => array('mini_layouts_admin_configure_cancel'),
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
function mini_layouts_admin_configure_cancel($form, &$form_state) {
  layout_clear_layout_tempstore($form_state['mini_layout']->name);
  $form_state['redirect'] = 'admin/structure/mini-layouts';
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
  foreach ($blocks as $name => $block) {
    if ($name != $form_state['values']['name'] && $block['info'] == $form_state['values']['info']) {
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
  $edit = $form_state['values'];
  $name = $form_state['values']['name'];
  $name = $name ? $name : preg_replace('/[^a-z0-9_]+/', '_', strtolower($edit['info']));

  $mini_layout = layout_tempstore_load($name);

  if (!$mini_layout) {
    $mini_layout = mini_layouts_load($name);
  }
  elseif (!$mini_layout) {
    $block = array(
      'name' => $name,
      'info' => '',
      'title' => '',
      'description' => '',
      'layout_template' => 'boxton',
    );
    $mini_layout = new MiniLayout($block);
  }

  foreach (array('info', 'description', 'title', 'body') as $key) {
    if (isset($edit[$key])) {
      $mini_layout->{$key} = $edit[$key];
    }
  }

  $mini_layout->setLayoutTemplate($edit['layout_template']);
  $mini_layout->save();

  // Reset the static cache on the block list so this block is picked up.
  cache()->delete('layout:mini_layouts:config');
  layout_clear_layout_tempstore($mini_layout->name);
  backdrop_static_reset('mini_layouts_block_info');
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
 * Saves the new mini layouts block.
 *
 * @see mini_layouts_add_block_form()
 * @see mini_layouts_add_block_form_validate()
 */
function mini_layouts_add_block_form_submit($form, &$form_state) {
  mini_layouts_block_layout_save($form_state['values'], $form_state['values']['name']);
  backdrop_set_message(t('The block has been created.'));
  $form_state['redirect'] = 'admin/structure/mini-layouts';
}

/**
 * Form constructor for the mini layouts block deletion form.
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 * @param $name
 *   The unique ID of the block within the context of $module.
 *
 * @see mini_layouts_menu()
 * @see mini_layouts_block_delete_submit()
 */
function mini_layouts_block_delete($form, &$form_state, $name) {
  $mini_layout = mini_layouts_load($name);
  $form['info'] = array('#type' => 'hidden', '#value' => $mini_layout->info);
  $form['name'] = array('#type' => 'hidden', '#value' => $name);

  return confirm_form($form, t('Are you sure you want to delete the block %name?', array('%name' => $mini_layout->info)), 'admin/structure/mini-layouts', '', t('Delete'), t('Cancel'));
}

/**
 * Form submission handler for mini_layouts_block_delete().
 *
 * @param array $form
 *   An associative array containing the structure of a portion of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @see mini_layouts_block_delete()
 */
function mini_layouts_block_delete_submit($form, &$form_state) {
  $layout = mini_layouts_load($form_state['values']['name']);
  $layout->delete();
  backdrop_set_message(t('The block %name has been removed.', array('%name' => $form_state['values']['info'])));
  $form_state['redirect'] = 'admin/structure/mini-layouts';
}

