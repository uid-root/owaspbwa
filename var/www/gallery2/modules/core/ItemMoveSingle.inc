<?php
/*
 * $RCSfile: ItemMoveSingle.inc,v $
 *
 * Gallery - a web based photo album viewer and editor
 * Copyright (C) 2000-2006 Bharat Mediratta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @version $Revision: 1.11 $ $Date: 2006/01/10 04:39:20 $
 * @package GalleryCore
 * @subpackage UserInterface
 * @author Bharat Mediratta <bharat@menalto.com>
 */

/**
 * This controller will handle moving one or more items from one album to
 * another.
 *
 * @package GalleryCore
 * @subpackage UserInterface
 *
 */
class ItemMoveSingleController extends GalleryController {

    /**
     * Move the selected items into the destination album, if all the
     * permissions are set correctly.
     */
    function handleRequest($form) {
	global $gallery;

	$itemId = GalleryUtilities::getRequestVariables('itemId');
	list ($ret, $item) = GalleryCoreApi::loadEntitiesById($itemId);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$status = array();
	$error = array();

	if (isset($form['action']['move'])) {
	    if (empty($form['destination'])) {
		$error[] = 'form[error][destination][empty]';
	    }

	    $ret = GalleryCoreApi::assertHasItemPermission($itemId, 'core.delete');
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }

	    if (empty($error)) {
		$newParentId = $form['destination'];

		/*
		 * Assert that our permissions are correct.  The view should never try to make a
		 * move that isn't legal so we can just bail if there's an inconsistency.
		 */
		if (GalleryUtilities::isA($item, 'GalleryDataItem')) {
		    $ret = GalleryCoreApi::assertHasItemPermission($newParentId,
								   'core.addDataItem');
		    if ($ret) {
			return array($ret->wrap(__FILE__, __LINE__), null);
		    }
		} else if (GalleryUtilities::isA($item, 'GalleryAlbumItem')) {
		    $ret = GalleryCoreApi::assertHasItemPermission($newParentId,
								   'core.addAlbumItem');
		    if ($ret) {
			return array($ret->wrap(__FILE__, __LINE__), null);
		    }
		} else {
		    /* The view should never let this happen */
		    return array(GalleryCoreApi::error(ERROR_BAD_DATA_TYPE, __FILE__, __LINE__),
				 null);
		}
	    }

	    /* Make sure the destination is the right type of item */
	    if (empty($error)) {
		list ($ret, $newParent) = GalleryCoreApi::loadEntitiesById($newParentId);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		if (!GalleryUtilities::isA($newParent, 'GalleryAlbumItem')) {
		    /* The view should never let this happen */
		    return array(GalleryCoreApi::error(ERROR_BAD_DATA_TYPE, __FILE__, __LINE__),
				 null);
		}
	    }

	    /* Make sure we don't have recursive moves */
	    if (empty($error)) {
		list($ret, $newParentAncestorIds) =
		    GalleryCoreApi::fetchParentSequence($newParentId);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		if ($itemId == $newParentId || in_array($itemId, $newParentAncestorIds)) {
		    $error[] = 'form[error][destination][selfMove]';
		}
	    }

	    if (empty($error)) {
		/*
		 * Read lock both parent hierarchies
		 * TODO: Optimize this
		 */
		list ($ret, $lockIds[]) = GalleryCoreApi::acquireReadLockParents($newParentId);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		$oldParentId = $item->getParentId();
		list ($ret, $lockIds[]) = GalleryCoreApi::acquireReadLockParents($oldParentId);
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lockIds);
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		list ($ret, $lockIds[]) =
		    GalleryCoreApi::acquireReadLock(array($newParentId, $oldParentId));
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lockIds);
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		/* Write lock the item we're moving */
		list ($ret, $lockIds[]) = GalleryCoreApi::acquireWriteLock($itemId);
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lockIds);
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		/* Refresh the item in case it changed before it was locked */
		list ($ret, $item) = $item->refresh();
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lockIds);
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		/* Do the move */
		$ret = $item->move($newParentId);
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lockIds);
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		$ret = $item->save();
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lockIds);
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		if (GalleryUtilities::isA($item, 'GalleryDataItem')) {
		    /* Update for derivative preferences of new parent */
		    $ret = GalleryCoreApi::addExistingItemToAlbum($item, $newParentId);
		    if ($ret) {
			GalleryCoreApi::releaseLocks($lockIds);
			return array($ret->wrap(__FILE__, __LINE__), null);
		    }
		}

		/* Release all locks */
		$ret = GalleryCoreApi::releaseLocks($lockIds);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		/* Fix thumbnail integrity */
		list ($ret, $success) = GalleryCoreApi::guaranteeAlbumHasThumbnail($oldParentId);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		$status['moved'] = 1;

		/* Figure out where to redirect upon success */
		$redirect['view'] = 'core.ItemAdmin';
		$redirect['subView'] = 'core.ItemMoveSingle';
		$redirect['itemId'] = $itemId;
	    }
	}

	if (!empty($redirect)) {
	    $results['redirect'] = $redirect;
	} else {
	    $results['delegate']['view'] = 'core.ItemAdmin';
	    $results['delegate']['subView'] = 'core.ItemMoveSingle';
	}
	$results['status'] = $status;
	$results['error'] = $error;

	return array(null, $results);
    }
}

/**
 * This view will prompt for which files to move/copy and which is the
 * destination.
 *
 * @package GalleryCore
 * @subpackage UserInterface
 */
class ItemMoveSingleView extends GalleryView {

    /**
     * @see GalleryView::loadTemplate
     */
    function loadTemplate(&$template, &$form) {
	global $gallery;

	/* itemId is the album where we want to move items from */
	$itemId = GalleryUtilities::getRequestVariables('itemId');
	if ($form['formName'] != 'ItemMoveSingle') {
	    /* First time around, load the form with item data */
	    $form['destination'] = '';
	    $form['formName'] = 'ItemMoveSingle';
	}

	list ($ret, $item) = GalleryCoreApi::loadEntitiesById($itemId);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	if (GalleryUtilities::isA($item, 'GalleryDataItem')) {
	    $permission = 'core.addDataItem';
	} else if (GalleryUtilities::isA($item, 'GalleryAlbumItem')) {
	    $permission = 'core.addAlbumItem';
	} else {
	    return array(GalleryCoreApi::error(ERROR_BAD_DATA_TYPE, __FILE__, __LINE__), null);
	}

	/* Get ids of all all albums where we can add this items */
	list ($ret, $albumIds) = GalleryCoreApi::fetchAllItemIds('GalleryAlbumItem', $permission);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	/* Load all the album entities */
	list ($ret, $albums) = GalleryCoreApi::loadEntitiesById($albumIds);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$albumTree = GalleryUtilities::createAlbumTree($albums);

	$ItemMoveSingle = array();
	$ItemMoveSingle['albumTree'] = $albumTree;
	$ItemMoveSingle['itemTypeNames'] = $item->itemTypeName();

	$template->setVariable('ItemMoveSingle', $ItemMoveSingle);
	$template->setVariable('controller', 'core.ItemMoveSingle');

	return array(null,
		     array('body' => 'modules/core/templates/ItemMoveSingle.tpl'));
    }

    /**
     * @see GalleryView::getViewDescription()
     */
    function getViewDescription() {
	list ($ret, $core) = GalleryCoreApi::loadPlugin('module', 'core');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $item) = $this->_getItem();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$itemTypeNames = $item->itemTypeName(true);

	return array(null,
		     $core->translate(array('text' => 'move %s', 'arg1' => $itemTypeNames[1])));
    }
}
?>
