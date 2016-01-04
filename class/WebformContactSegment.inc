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
        $contacts = _pum_contactsegment_drupal_webform_get_contact($component);
        $role = _pum_contactsegment_drupal_webform_get_role($component);
        foreach($submission as $contactsegment_id) {
            $parameters = array();
            $parameters['role_value'] = $role;
            $parameters['segment_id'] = $contactsegment_id;
            foreach ($contacts as $key) {
                if (isset($this->submission->civicrm['contact_id'][$key])) {
                    $parameters['contact_id'] = $this->submission->civicrm['contact_id'][$key];
                    civicrm_api3('ContactSegment', 'create', $parameters);
                }
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