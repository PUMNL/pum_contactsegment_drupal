<?php

/**
 * @author Jaap Jansma (CiviCooP) <jaap.jansma@civicoop.org>
 * @license http://www.gnu.org/licenses/agpl-3.0.html
 */
class WebformContactSegment
{

    private $submission;
    private $node;

    /**
     *
     * @param int $nid - Node ID of the webform
     * @param int $sid - ID of the submitted result
     */
    public function __construct($nid, $sid) {
        $this->node = node_load($nid);
        $this->submission = webform_get_submission($nid, $sid);
    }

    public function save() {
        if (!$this->isValidSubmission()) {
            return;
        }

        foreach ($this->node->webform['components'] as $cid => $component) {
            if (self::checkComponentForContactSegment($component)) {
                $this->saveContactSegment($this->submission->data[$cid], $component);
            }
        }
    }

    protected function saveContactSegment($submission, $component) {
        $start_date = new DateTime();
        $contacts = _pum_contactsegment_drupal_webform_get_contact($component);
        $role = _pum_contactsegment_drupal_webform_get_role($component);
        $is_main_sector = _pum_contactsegment_drupal_webform_is_main_sector($component);
        foreach($submission as $contactsegment_id) {
            $parameters = array();
            $parameters['start_date'] = $start_date->format('Ymd');
            $parameters['role_value'] = $role;
            $parameters['segment_id'] = $contactsegment_id;
            if ($is_main_sector) {
                $parameters['is_main'] = 1;
            }
            foreach ($contacts as $key) {
                if (isset($this->submission->civicrm['contact_id'][$key])) {
                    $contact_id = $this->submission->civicrm['contact_id'][$key];
                    $this->checkUniqueAndEnd($contactsegment_id, $role, $contact_id);
                    $parameters['contact_id'] = $contact_id;
                    civicrm_api3('ContactSegment', 'create', $parameters);
                }
            }
        }
    }

    protected function checkUniqueAndEnd($segment_id, $role, $contact_id) {
        $start_date = new DateTime();
        $segment = civicrm_api3('Segment', 'getsingle', array('id' => $segment_id));
        $segment_type = empty($segment['parent_id']) ? 'parent' : 'child';
        $is_unqiue = pum_contactsegment_drupal_is_role_unique(pum_contactsegment_drupal_get_role_name($role), $segment_type);
        if ($is_unqiue) {
            $current_id = pum_contactsegment_drupal_has_segment_active_role($role, $segment_id, $contact_id, $start_date);
            if ($current_id) {
                $endParams = array();
                $endParams['id'] = $current_id;
                $endParams['end_date'] = $start_date->format('Ymd');
                civicrm_api3('ContactSegment', 'create', $endParams);
            }
        }
    }

    public static function checkComponentForContactSegment($component) {
        if ($component['type'] != 'select' || !$component['extra']['webform_civicrm_contactsegment_enabled']) {
            return false;
        }
        return true;
    }

    /**
     * Checks if the submitted data is linked to civicrm
     */
    protected function isValidSubmission() {
        if (!isset($this->node->webform)) {
            return false;
        }

        if (!isset($this->node->webform['components'])) {
            return false;
        }

        //do a check if this is a civicrm submission
        if (!isset($this->submission->civicrm)) {
            return false;
        }

        if (!isset($this->submission->civicrm['contact_id'])) {
            return false;
        }

        if (count($this->submission->civicrm['contact_id']) == 0) {
            return false;
        }

        return true;
    }

}