/*************************************************************************/
/*  tile_map_editor.cpp                                                  */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "tile_map_editor.h"

#include "tiles_editor_plugin.h"

#include "editor/editor_resource_preview.h"
#include "editor/editor_scale.h"
#include "editor/plugins/canvas_item_editor_plugin.h"

#include "scene/gui/center_container.h"
#include "scene/gui/split_container.h"

#include "core/input/input.h"
#include "core/math/geometry_2d.h"
#include "core/os/keyboard.h"

void TileMapEditorTilesPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED:
			select_tool_button->set_icon(get_theme_icon(SNAME("ToolSelect"), SNAME("EditorIcons")));
			paint_tool_button->set_icon(get_theme_icon(SNAME("Edit"), SNAME("EditorIcons")));
			line_tool_button->set_icon(get_theme_icon(SNAME("CurveLinear"), SNAME("EditorIcons")));
			rect_tool_button->set_icon(get_theme_icon(SNAME("Rectangle"), SNAME("EditorIcons")));
			bucket_tool_button->set_icon(get_theme_icon(SNAME("Bucket"), SNAME("EditorIcons")));

			picker_button->set_icon(get_theme_icon(SNAME("ColorPick"), SNAME("EditorIcons")));
			erase_button->set_icon(get_theme_icon(SNAME("Eraser"), SNAME("EditorIcons")));

			missing_atlas_texture_icon = get_theme_icon(SNAME("TileSet"), SNAME("EditorIcons"));
			break;
		case NOTIFICATION_VISIBILITY_CHANGED:
			_stop_dragging();
			break;
	}
}

void TileMapEditorTilesPlugin::tile_set_changed() {
	_update_fix_selected_and_hovered();
	_update_tile_set_sources_list();
	_update_bottom_panel();
}

void TileMapEditorTilesPlugin::_on_random_tile_checkbox_toggled(bool p_pressed) {
	scatter_spinbox->set_editable(p_pressed);
}

void TileMapEditorTilesPlugin::_on_scattering_spinbox_changed(double p_value) {
	scattering = p_value;
}

void TileMapEditorTilesPlugin::_update_toolbar() {
	// Stop draggig if needed.
	_stop_dragging();

	// Hide all settings.
	for (int i = 0; i < tools_settings->get_child_count(); i++) {
		Object::cast_to<CanvasItem>(tools_settings->get_child(i))->hide();
	}

	// Show only the correct settings.
	if (tool_buttons_group->get_pressed_button() == select_tool_button) {
	} else if (tool_buttons_group->get_pressed_button() == paint_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	} else if (tool_buttons_group->get_pressed_button() == line_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	} else if (tool_buttons_group->get_pressed_button() == rect_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		bucket_continuous_checkbox->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	}
}

Control *TileMapEditorTilesPlugin::get_toolbar() const {
	return toolbar;
}

void TileMapEditorTilesPlugin::_update_tile_set_sources_list() {
	// Update the sources.
	int old_current = sources_list->get_current();
	sources_list->clear();

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	for (int i = 0; i < tile_set->get_source_count(); i++) {
		int source_id = tile_set->get_source_id(i);

		TileSetSource *source = *tile_set->get_source(source_id);

		Ref<Texture2D> texture;
		String item_text;

		// Atlas source.
		TileSetAtlasSource *atlas_source = Object::cast_to<TileSetAtlasSource>(source);
		if (atlas_source) {
			texture = atlas_source->get_texture();
			if (texture.is_valid()) {
				item_text = vformat("%s (ID: %d)", texture->get_path().get_file(), source_id);
			} else {
				item_text = vformat("No Texture Atlas Source (ID: %d)", source_id);
			}
		}

		// Scene collection source.
		TileSetScenesCollectionSource *scene_collection_source = Object::cast_to<TileSetScenesCollectionSource>(source);
		if (scene_collection_source) {
			texture = get_theme_icon(SNAME("PackedScene"), SNAME("EditorIcons"));
			item_text = vformat(TTR("Scene Collection Source (ID: %d)"), source_id);
		}

		// Use default if not valid.
		if (item_text.is_empty()) {
			item_text = vformat(TTR("Unknown Type Source (ID: %d)"), source_id);
		}
		if (!texture.is_valid()) {
			texture = missing_atlas_texture_icon;
		}

		sources_list->add_item(item_text, texture);
		sources_list->set_item_metadata(i, source_id);
	}

	if (sources_list->get_item_count() > 0) {
		if (old_current > 0) {
			// Keep the current selected item if needed.
			sources_list->set_current(CLAMP(old_current, 0, sources_list->get_item_count() - 1));
		} else {
			sources_list->set_current(0);
		}
		sources_list->emit_signal(SNAME("item_selected"), sources_list->get_current());
	}

	// Synchronize
	TilesEditor::get_singleton()->set_sources_lists_current(sources_list->get_current());
}

void TileMapEditorTilesPlugin::_update_bottom_panel() {
	// Update the atlas display.
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index >= 0 && source_index < sources_list->get_item_count()) {
		atlas_sources_split_container->show();
		missing_source_label->hide();

		int source_id = sources_list->get_item_metadata(source_index);
		TileSetSource *source = *tile_set->get_source(source_id);
		TileSetAtlasSource *atlas_source = Object::cast_to<TileSetAtlasSource>(source);
		TileSetScenesCollectionSource *scenes_collection_source = Object::cast_to<TileSetScenesCollectionSource>(source);

		if (atlas_source) {
			tile_atlas_view->show();
			scene_tiles_list->hide();
			invalid_source_label->hide();
			_update_atlas_view();
		} else if (scenes_collection_source) {
			tile_atlas_view->hide();
			scene_tiles_list->show();
			invalid_source_label->hide();
			_update_scenes_collection_view();
		} else {
			tile_atlas_view->hide();
			scene_tiles_list->hide();
			invalid_source_label->show();
		}
	} else {
		atlas_sources_split_container->hide();
		missing_source_label->show();

		tile_atlas_view->hide();
		scene_tiles_list->hide();
		invalid_source_label->hide();
	}
}

void TileMapEditorTilesPlugin::_update_atlas_view() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(sources_list->get_current());
	TileSetSource *source = *tile_set->get_source(source_id);
	TileSetAtlasSource *atlas_source = Object::cast_to<TileSetAtlasSource>(source);
	ERR_FAIL_COND(!atlas_source);

	tile_atlas_view->set_atlas_source(*tile_map->get_tileset(), atlas_source, source_id);
	TilesEditor::get_singleton()->synchronize_atlas_view(tile_atlas_view);
	tile_atlas_control->update();
}

void TileMapEditorTilesPlugin::_update_scenes_collection_view() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(sources_list->get_current());
	TileSetSource *source = *tile_set->get_source(source_id);
	TileSetScenesCollectionSource *scenes_collection_source = Object::cast_to<TileSetScenesCollectionSource>(source);
	ERR_FAIL_COND(!scenes_collection_source);

	// Clear the list.
	scene_tiles_list->clear();

	// Rebuild the list.
	for (int i = 0; i < scenes_collection_source->get_scene_tiles_count(); i++) {
		int scene_id = scenes_collection_source->get_scene_tile_id(i);

		Ref<PackedScene> scene = scenes_collection_source->get_scene_tile_scene(scene_id);

		int item_index = 0;
		if (scene.is_valid()) {
			item_index = scene_tiles_list->add_item(vformat("%s (Path: %s, ID: %d)", scene->get_path().get_file().get_basename(), scene->get_path(), scene_id));
			Variant udata = i;
			EditorResourcePreview::get_singleton()->queue_edited_resource_preview(scene, this, "_scene_thumbnail_done", udata);
		} else {
			item_index = scene_tiles_list->add_item(TTR("Tile with Invalid Scene"), get_theme_icon(SNAME("PackedScene"), SNAME("EditorIcons")));
		}
		scene_tiles_list->set_item_metadata(item_index, scene_id);

		// Check if in selection.
		if (tile_set_selection.has(TileMapCell(source_id, Vector2i(), scene_id))) {
			scene_tiles_list->select(item_index, false);
		}
	}

	// Icon size update.
	int int_size = int(EditorSettings::get_singleton()->get("filesystem/file_dialog/thumbnail_size")) * EDSCALE;
	scene_tiles_list->set_fixed_icon_size(Vector2(int_size, int_size));
}

void TileMapEditorTilesPlugin::_scene_thumbnail_done(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, Variant p_ud) {
	int index = p_ud;

	if (index >= 0 && index < scene_tiles_list->get_item_count()) {
		scene_tiles_list->set_item_icon(index, p_preview);
	}
}

void TileMapEditorTilesPlugin::_scenes_list_multi_selected(int p_index, bool p_selected) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Add or remove the Tile form the selection.
	int scene_id = scene_tiles_list->get_item_metadata(p_index);
	int source_id = sources_list->get_item_metadata(sources_list->get_current());
	TileSetSource *source = *tile_set->get_source(source_id);
	TileSetScenesCollectionSource *scenes_collection_source = Object::cast_to<TileSetScenesCollectionSource>(source);
	ERR_FAIL_COND(!scenes_collection_source);

	TileMapCell selected = TileMapCell(source_id, Vector2i(), scene_id);

	// Clear the selection if shift is not pressed.
	if (!Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
		tile_set_selection.clear();
	}

	if (p_selected) {
		tile_set_selection.insert(selected);
	} else {
		if (tile_set_selection.has(selected)) {
			tile_set_selection.erase(selected);
		}
	}

	_update_selection_pattern_from_tileset_selection();
}

void TileMapEditorTilesPlugin::_scenes_list_nothing_selected() {
	scene_tiles_list->deselect_all();
	tile_set_selection.clear();
	tile_map_selection.clear();
	selection_pattern->clear();
	_update_selection_pattern_from_tileset_selection();
}

bool TileMapEditorTilesPlugin::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {
	if (!is_visible_in_tree()) {
		// If the bottom editor is not visible, we ignore inputs.
		return false;
	}

	if (CanvasItemEditor::get_singleton()->get_current_tool() != CanvasItemEditor::TOOL_SELECT) {
		return false;
	}

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return false;
	}

	if (tile_map_layer < 0) {
		return false;
	}
	ERR_FAIL_INDEX_V(tile_map_layer, tile_map->get_layers_count(), false);

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return false;
	}

	// Shortcuts
	if (ED_IS_SHORTCUT("tiles_editor/cut", p_event) || ED_IS_SHORTCUT("tiles_editor/copy", p_event)) {
		// Fill in the clipboard.
		if (!tile_map_selection.is_empty()) {
			memdelete(tile_map_clipboard);
			TypedArray<Vector2i> coords_array;
			for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
				coords_array.push_back(E->get());
			}
			tile_map_clipboard = tile_map->get_pattern(tile_map_layer, coords_array);
		}

		if (ED_IS_SHORTCUT("tiles_editor/cut", p_event)) {
			// Delete selected tiles.
			if (!tile_map_selection.is_empty()) {
				undo_redo->create_action(TTR("Delete tiles"));
				for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
					undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->get(), TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
					undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->get(), tile_map->get_cell_source_id(tile_map_layer, E->get()), tile_map->get_cell_atlas_coords(tile_map_layer, E->get()), tile_map->get_cell_alternative_tile(tile_map_layer, E->get()));
				}
				undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());
				tile_map_selection.clear();
				undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
				undo_redo->commit_action();
			}
		}

		return true;
	}
	if (ED_IS_SHORTCUT("tiles_editor/paste", p_event)) {
		if (drag_type == DRAG_TYPE_NONE) {
			drag_type = DRAG_TYPE_CLIPBOARD_PASTE;
		}
		CanvasItemEditor::get_singleton()->update_viewport();
		return true;
	}
	if (ED_IS_SHORTCUT("tiles_editor/cancel", p_event)) {
		if (drag_type == DRAG_TYPE_CLIPBOARD_PASTE) {
			drag_type = DRAG_TYPE_NONE;
			CanvasItemEditor::get_singleton()->update_viewport();
			return true;
		}
	}
	if (ED_IS_SHORTCUT("tiles_editor/delete", p_event)) {
		// Delete selected tiles.
		if (!tile_map_selection.is_empty()) {
			undo_redo->create_action(TTR("Delete tiles"));
			for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->get(), TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->get(), tile_map->get_cell_source_id(tile_map_layer, E->get()), tile_map->get_cell_atlas_coords(tile_map_layer, E->get()), tile_map->get_cell_alternative_tile(tile_map_layer, E->get()));
			}
			undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			tile_map_selection.clear();
			undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			undo_redo->commit_action();
		}
		return true;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mm->get_position());

		switch (drag_type) {
			case DRAG_TYPE_PAINT: {
				Map<Vector2i, TileMapCell> to_draw = _draw_line(drag_start_mouse_pos, drag_last_mouse_pos, mpos);
				for (Map<Vector2i, TileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
					if (!erase_button->is_pressed() && E->get().source_id == TileSet::INVALID_SOURCE) {
						continue;
					}
					Vector2i coords = E->key();
					if (!drag_modified.has(coords)) {
						drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
					}
					tile_map->set_cell(tile_map_layer, coords, E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
				}
			} break;
			case DRAG_TYPE_BUCKET: {
				Vector<Vector2i> line = TileMapEditor::get_line(tile_map, tile_map->world_to_map(drag_last_mouse_pos), tile_map->world_to_map(mpos));
				for (int i = 0; i < line.size(); i++) {
					if (!drag_modified.has(line[i])) {
						Map<Vector2i, TileMapCell> to_draw = _draw_bucket_fill(line[i], bucket_continuous_checkbox->is_pressed());
						for (Map<Vector2i, TileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
							if (!erase_button->is_pressed() && E->get().source_id == TileSet::INVALID_SOURCE) {
								continue;
							}
							Vector2i coords = E->key();
							if (!drag_modified.has(coords)) {
								drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
							}
							tile_map->set_cell(tile_map_layer, coords, E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
						}
					}
				}
			} break;
			default:
				break;
		}
		drag_last_mouse_pos = mpos;
		CanvasItemEditor::get_singleton()->update_viewport();

		return true;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mb->get_position());

		if (mb->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (mb->is_pressed()) {
				// Pressed
				if (drag_type == DRAG_TYPE_CLIPBOARD_PASTE) {
					// Do nothing.
				} else if (tool_buttons_group->get_pressed_button() == select_tool_button) {
					drag_start_mouse_pos = mpos;
					if (tile_map_selection.has(tile_map->world_to_map(drag_start_mouse_pos)) && !mb->is_shift_pressed()) {
						// Move the selection
						drag_type = DRAG_TYPE_MOVE;
						drag_modified.clear();
						for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
							Vector2i coords = E->get();
							drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
							tile_map->set_cell(tile_map_layer, coords, TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
						}
					} else {
						// Select tiles
						drag_type = DRAG_TYPE_SELECT;
					}
				} else {
					// Check if we are picking a tile.
					if (picker_button->is_pressed()) {
						drag_type = DRAG_TYPE_PICK;
						drag_start_mouse_pos = mpos;
					} else {
						// Paint otherwise.
						if (tool_buttons_group->get_pressed_button() == paint_tool_button) {
							drag_type = DRAG_TYPE_PAINT;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
							Map<Vector2i, TileMapCell> to_draw = _draw_line(drag_start_mouse_pos, mpos, mpos);
							for (Map<Vector2i, TileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
								if (!erase_button->is_pressed() && E->get().source_id == TileSet::INVALID_SOURCE) {
									continue;
								}
								Vector2i coords = E->key();
								if (!drag_modified.has(coords)) {
									drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
								}
								tile_map->set_cell(tile_map_layer, coords, E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
							}
						} else if (tool_buttons_group->get_pressed_button() == line_tool_button) {
							drag_type = DRAG_TYPE_LINE;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
						} else if (tool_buttons_group->get_pressed_button() == rect_tool_button) {
							drag_type = DRAG_TYPE_RECT;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
						} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button) {
							drag_type = DRAG_TYPE_BUCKET;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
							Vector<Vector2i> line = TileMapEditor::get_line(tile_map, tile_map->world_to_map(drag_last_mouse_pos), tile_map->world_to_map(mpos));
							for (int i = 0; i < line.size(); i++) {
								if (!drag_modified.has(line[i])) {
									Map<Vector2i, TileMapCell> to_draw = _draw_bucket_fill(line[i], bucket_continuous_checkbox->is_pressed());
									for (Map<Vector2i, TileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
										if (!erase_button->is_pressed() && E->get().source_id == TileSet::INVALID_SOURCE) {
											continue;
										}
										Vector2i coords = E->key();
										if (!drag_modified.has(coords)) {
											drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
										}
										tile_map->set_cell(tile_map_layer, coords, E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
									}
								}
							}
						}
					}
				}

			} else {
				// Released
				_stop_dragging();
			}

			CanvasItemEditor::get_singleton()->update_viewport();

			return true;
		}
		drag_last_mouse_pos = mpos;
	}

	return false;
}

void TileMapEditorTilesPlugin::forward_canvas_draw_over_viewport(Control *p_overlay) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	if (tile_map_layer < 0) {
		return;
	}
	ERR_FAIL_INDEX(tile_map_layer, tile_map->get_layers_count());

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (!tile_map->is_visible_in_tree()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Vector2i tile_shape_size = tile_set->get_tile_size();

	// Draw the selection.
	if (is_visible_in_tree() && tool_buttons_group->get_pressed_button() == select_tool_button) {
		// In select mode, we only draw the current selection if we are modifying it (pressing control or shift).
		if (drag_type == DRAG_TYPE_MOVE || (drag_type == DRAG_TYPE_SELECT && !Input::get_singleton()->is_key_pressed(KEY_CTRL) && !Input::get_singleton()->is_key_pressed(KEY_SHIFT))) {
			// Do nothing
		} else {
			Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
			Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
			tile_map->draw_cells_outline(p_overlay, tile_map_selection, selection_color, xform);
		}
	}

	// Handle the preview of the tiles to be placed.
	if (is_visible_in_tree() && has_mouse) { // Only if the tilemap editor is opened and the viewport is hovered.
		Map<Vector2i, TileMapCell> preview;
		Rect2i drawn_grid_rect;

		if (drag_type == DRAG_TYPE_PICK) {
			// Draw the area being picvked.
			Rect2i rect = Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(drag_last_mouse_pos) - tile_map->world_to_map(drag_start_mouse_pos)).abs();
			rect.size += Vector2i(1, 1);
			for (int x = rect.position.x; x < rect.get_end().x; x++) {
				for (int y = rect.position.y; y < rect.get_end().y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (tile_map->get_cell_source_id(tile_map_layer, coords) != TileSet::INVALID_SOURCE) {
						Transform2D tile_xform;
						tile_xform.set_origin(tile_map->map_to_world(coords));
						tile_xform.set_scale(tile_shape_size);
						tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0), false);
					}
				}
			}
		} else if (drag_type == DRAG_TYPE_SELECT) {
			// Draw the area being selected.
			Rect2i rect = Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(drag_last_mouse_pos) - tile_map->world_to_map(drag_start_mouse_pos)).abs();
			rect.size += Vector2i(1, 1);
			Set<Vector2i> to_draw;
			for (int x = rect.position.x; x < rect.get_end().x; x++) {
				for (int y = rect.position.y; y < rect.get_end().y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (tile_map->get_cell_source_id(tile_map_layer, coords) != TileSet::INVALID_SOURCE) {
						to_draw.insert(coords);
					}
				}
			}
			tile_map->draw_cells_outline(p_overlay, to_draw, Color(1.0, 1.0, 1.0), xform);
		} else if (drag_type == DRAG_TYPE_MOVE) {
			// Preview when moving.
			Vector2i top_left;
			if (!tile_map_selection.is_empty()) {
				top_left = tile_map_selection.front()->get();
			}
			for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
				top_left = top_left.min(E->get());
			}
			Vector2i offset = drag_start_mouse_pos - tile_map->map_to_world(top_left);
			offset = tile_map->world_to_map(drag_last_mouse_pos - offset) - tile_map->world_to_map(drag_start_mouse_pos - offset);

			TypedArray<Vector2i> selection_used_cells = selection_pattern->get_used_cells();
			for (int i = 0; i < selection_used_cells.size(); i++) {
				Vector2i coords = tile_map->map_pattern(offset + top_left, selection_used_cells[i], selection_pattern);
				preview[coords] = TileMapCell(selection_pattern->get_cell_source_id(selection_used_cells[i]), selection_pattern->get_cell_atlas_coords(selection_used_cells[i]), selection_pattern->get_cell_alternative_tile(selection_used_cells[i]));
			}
		} else if (drag_type == DRAG_TYPE_CLIPBOARD_PASTE) {
			// Preview when pasting.
			Vector2 mouse_offset = (Vector2(tile_map_clipboard->get_size()) / 2.0 - Vector2(0.5, 0.5)) * tile_set->get_tile_size();
			TypedArray<Vector2i> clipboard_used_cells = tile_map_clipboard->get_used_cells();
			for (int i = 0; i < clipboard_used_cells.size(); i++) {
				Vector2i coords = tile_map->map_pattern(tile_map->world_to_map(drag_last_mouse_pos - mouse_offset), clipboard_used_cells[i], tile_map_clipboard);
				preview[coords] = TileMapCell(tile_map_clipboard->get_cell_source_id(clipboard_used_cells[i]), tile_map_clipboard->get_cell_atlas_coords(clipboard_used_cells[i]), tile_map_clipboard->get_cell_alternative_tile(clipboard_used_cells[i]));
			}
		} else if (!picker_button->is_pressed()) {
			bool expand_grid = false;
			if (tool_buttons_group->get_pressed_button() == paint_tool_button && drag_type == DRAG_TYPE_NONE) {
				// Preview for a single pattern.
				preview = _draw_line(drag_last_mouse_pos, drag_last_mouse_pos, drag_last_mouse_pos);
				expand_grid = true;
			} else if (tool_buttons_group->get_pressed_button() == line_tool_button) {
				if (drag_type == DRAG_TYPE_NONE) {
					// Preview for a single pattern.
					preview = _draw_line(drag_last_mouse_pos, drag_last_mouse_pos, drag_last_mouse_pos);
					expand_grid = true;
				} else if (drag_type == DRAG_TYPE_LINE) {
					// Preview for a line pattern.
					preview = _draw_line(drag_start_mouse_pos, drag_start_mouse_pos, drag_last_mouse_pos);
					expand_grid = true;
				}
			} else if (tool_buttons_group->get_pressed_button() == rect_tool_button && drag_type == DRAG_TYPE_RECT) {
				// Preview for a line pattern.
				preview = _draw_rect(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(drag_last_mouse_pos));
				expand_grid = true;
			} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button && drag_type == DRAG_TYPE_NONE) {
				// Preview for a line pattern.
				preview = _draw_bucket_fill(tile_map->world_to_map(drag_last_mouse_pos), bucket_continuous_checkbox->is_pressed());
			}

			// Expand the grid if needed
			if (expand_grid && !preview.is_empty()) {
				drawn_grid_rect = Rect2i(preview.front()->key(), Vector2i(1, 1));
				for (Map<Vector2i, TileMapCell>::Element *E = preview.front(); E; E = E->next()) {
					drawn_grid_rect.expand_to(E->key());
				}
			}
		}

		if (!preview.is_empty()) {
			const int fading = 5;

			// Draw the lines of the grid behind the preview.
			bool display_grid = EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid");
			if (display_grid) {
				Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
				if (drawn_grid_rect.size.x > 0 && drawn_grid_rect.size.y > 0) {
					drawn_grid_rect = drawn_grid_rect.grow(fading);
					for (int x = drawn_grid_rect.position.x; x < (drawn_grid_rect.position.x + drawn_grid_rect.size.x); x++) {
						for (int y = drawn_grid_rect.position.y; y < (drawn_grid_rect.position.y + drawn_grid_rect.size.y); y++) {
							Vector2i pos_in_rect = Vector2i(x, y) - drawn_grid_rect.position;

							// Fade out the border of the grid.
							float left_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.x), 0.0f, 1.0f);
							float right_opacity = CLAMP(Math::inverse_lerp((float)drawn_grid_rect.size.x, (float)(drawn_grid_rect.size.x - fading), (float)pos_in_rect.x), 0.0f, 1.0f);
							float top_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.y), 0.0f, 1.0f);
							float bottom_opacity = CLAMP(Math::inverse_lerp((float)drawn_grid_rect.size.y, (float)(drawn_grid_rect.size.y - fading), (float)pos_in_rect.y), 0.0f, 1.0f);
							float opacity = CLAMP(MIN(left_opacity, MIN(right_opacity, MIN(top_opacity, bottom_opacity))) + 0.1, 0.0f, 1.0f);

							Transform2D tile_xform;
							tile_xform.set_origin(tile_map->map_to_world(Vector2(x, y)));
							tile_xform.set_scale(tile_shape_size);
							Color color = grid_color;
							color.a = color.a * opacity;
							tile_set->draw_tile_shape(p_overlay, xform * tile_xform, color, false);
						}
					}
				}
			}

			// Draw the preview.
			for (Map<Vector2i, TileMapCell>::Element *E = preview.front(); E; E = E->next()) {
				Transform2D tile_xform;
				tile_xform.set_origin(tile_map->map_to_world(E->key()));
				tile_xform.set_scale(tile_set->get_tile_size());
				if (!erase_button->is_pressed() && random_tile_checkbox->is_pressed()) {
					tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0, 0.5), true);
				} else {
					if (tile_set->has_source(E->get().source_id)) {
						TileSetSource *source = *tile_set->get_source(E->get().source_id);
						TileSetAtlasSource *atlas_source = Object::cast_to<TileSetAtlasSource>(source);
						if (atlas_source) {
							// Get tile data.
							TileData *tile_data = Object::cast_to<TileData>(atlas_source->get_tile_data(E->get().get_atlas_coords(), E->get().alternative_tile));

							// Compute the offset
							Rect2i source_rect = atlas_source->get_tile_texture_region(E->get().get_atlas_coords());
							Vector2i tile_offset = atlas_source->get_tile_effective_texture_offset(E->get().get_atlas_coords(), E->get().alternative_tile);

							// Compute the destination rectangle in the CanvasItem.
							Rect2 dest_rect;
							dest_rect.size = source_rect.size;

							bool transpose = tile_data->get_transpose();
							if (transpose) {
								dest_rect.position = (tile_map->map_to_world(E->key()) - Vector2(dest_rect.size.y, dest_rect.size.x) / 2 - tile_offset);
							} else {
								dest_rect.position = (tile_map->map_to_world(E->key()) - dest_rect.size / 2 - tile_offset);
							}

							dest_rect = xform.xform(dest_rect);

							if (tile_data->get_flip_h()) {
								dest_rect.size.x = -dest_rect.size.x;
							}

							if (tile_data->get_flip_v()) {
								dest_rect.size.y = -dest_rect.size.y;
							}

							// Get the tile modulation.
							Color modulate = tile_data->get_modulate();
							Color self_modulate = tile_map->get_self_modulate();
							modulate = Color(modulate.r * self_modulate.r, modulate.g * self_modulate.g, modulate.b * self_modulate.b, modulate.a * self_modulate.a);

							// Draw the tile.
							p_overlay->draw_texture_rect_region(atlas_source->get_texture(), dest_rect, source_rect, modulate * Color(1.0, 1.0, 1.0, 0.5), transpose, tile_set->is_uv_clipping());
						} else {
							tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0, 0.5), true);
						}
					} else {
						tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(0.0, 0.0, 0.0, 0.5), true);
					}
				}
			}
		}
	}
}

void TileMapEditorTilesPlugin::_mouse_exited_viewport() {
	has_mouse = false;
	CanvasItemEditor::get_singleton()->update_viewport();
}

TileMapCell TileMapEditorTilesPlugin::_pick_random_tile(const TileMapPattern *p_pattern) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return TileMapCell();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return TileMapCell();
	}

	TypedArray<Vector2i> used_cells = p_pattern->get_used_cells();
	double sum = 0.0;
	for (int i = 0; i < used_cells.size(); i++) {
		int source_id = p_pattern->get_cell_source_id(used_cells[i]);
		Vector2i atlas_coords = p_pattern->get_cell_atlas_coords(used_cells[i]);
		int alternative_tile = p_pattern->get_cell_alternative_tile(used_cells[i]);

		TileSetSource *source = *tile_set->get_source(source_id);
		TileSetAtlasSource *atlas_source = Object::cast_to<TileSetAtlasSource>(source);
		if (atlas_source) {
			TileData *tile_data = Object::cast_to<TileData>(atlas_source->get_tile_data(atlas_coords, alternative_tile));
			ERR_FAIL_COND_V(!tile_data, TileMapCell());
			sum += tile_data->get_probability();
		} else {
			sum += 1.0;
		}
	}

	double empty_probability = sum * scattering;
	double current = 0.0;
	double rand = Math::random(0.0, sum + empty_probability);
	for (int i = 0; i < used_cells.size(); i++) {
		int source_id = p_pattern->get_cell_source_id(used_cells[i]);
		Vector2i atlas_coords = p_pattern->get_cell_atlas_coords(used_cells[i]);
		int alternative_tile = p_pattern->get_cell_alternative_tile(used_cells[i]);

		TileSetSource *source = *tile_set->get_source(source_id);
		TileSetAtlasSource *atlas_source = Object::cast_to<TileSetAtlasSource>(source);
		if (atlas_source) {
			current += Object::cast_to<TileData>(atlas_source->get_tile_data(atlas_coords, alternative_tile))->get_probability();
		} else {
			current += 1.0;
		}

		if (current >= rand) {
			return TileMapCell(source_id, atlas_coords, alternative_tile);
		}
	}
	return TileMapCell();
}

Map<Vector2i, TileMapCell> TileMapEditorTilesPlugin::_draw_line(Vector2 p_start_drag_mouse_pos, Vector2 p_from_mouse_pos, Vector2 p_to_mouse_pos) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, TileMapCell>();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, TileMapCell>();
	}

	// Get or create the pattern.
	TileMapPattern erase_pattern;
	erase_pattern.set_cell(Vector2i(0, 0), TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
	TileMapPattern *pattern = erase_button->is_pressed() ? &erase_pattern : selection_pattern;

	Map<Vector2i, TileMapCell> output;
	if (!pattern->is_empty()) {
		// Paint the tiles on the tile map.
		if (!erase_button->is_pressed() && random_tile_checkbox->is_pressed()) {
			// Paint a random tile.
			Vector<Vector2i> line = TileMapEditor::get_line(tile_map, tile_map->world_to_map(p_from_mouse_pos), tile_map->world_to_map(p_to_mouse_pos));
			for (int i = 0; i < line.size(); i++) {
				output.insert(line[i], _pick_random_tile(pattern));
			}
		} else {
			// Paint the pattern.
			// If we paint several tiles, we virtually move the mouse as if it was in the center of the "brush"
			Vector2 mouse_offset = (Vector2(pattern->get_size()) / 2.0 - Vector2(0.5, 0.5)) * tile_set->get_tile_size();
			Vector2i last_hovered_cell = tile_map->world_to_map(p_from_mouse_pos - mouse_offset);
			Vector2i new_hovered_cell = tile_map->world_to_map(p_to_mouse_pos - mouse_offset);
			Vector2i drag_start_cell = tile_map->world_to_map(p_start_drag_mouse_pos - mouse_offset);

			TypedArray<Vector2i> used_cells = pattern->get_used_cells();
			Vector2i offset = Vector2i(Math::posmod(drag_start_cell.x, pattern->get_size().x), Math::posmod(drag_start_cell.y, pattern->get_size().y)); // Note: no posmodv for Vector2i for now. Meh.s
			Vector<Vector2i> line = TileMapEditor::get_line(tile_map, (last_hovered_cell - offset) / pattern->get_size(), (new_hovered_cell - offset) / pattern->get_size());
			for (int i = 0; i < line.size(); i++) {
				Vector2i top_left = line[i] * pattern->get_size() + offset;
				for (int j = 0; j < used_cells.size(); j++) {
					Vector2i coords = tile_map->map_pattern(top_left, used_cells[j], pattern);
					output.insert(coords, TileMapCell(pattern->get_cell_source_id(used_cells[j]), pattern->get_cell_atlas_coords(used_cells[j]), pattern->get_cell_alternative_tile(used_cells[j])));
				}
			}
		}
	}
	return output;
}

Map<Vector2i, TileMapCell> TileMapEditorTilesPlugin::_draw_rect(Vector2i p_start_cell, Vector2i p_end_cell) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, TileMapCell>();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, TileMapCell>();
	}

	// Create the rect to draw.
	Rect2i rect = Rect2i(p_start_cell, p_end_cell - p_start_cell).abs();
	rect.size += Vector2i(1, 1);

	// Get or create the pattern.
	TileMapPattern erase_pattern;
	erase_pattern.set_cell(Vector2i(0, 0), TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
	TileMapPattern *pattern = erase_button->is_pressed() ? &erase_pattern : selection_pattern;
	Map<Vector2i, TileMapCell> err_output;
	ERR_FAIL_COND_V(pattern->is_empty(), err_output);

	// Compute the offset to align things to the bottom or right.
	bool aligned_right = p_end_cell.x < p_start_cell.x;
	bool valigned_bottom = p_end_cell.y < p_start_cell.y;
	Vector2i offset = Vector2i(aligned_right ? -(pattern->get_size().x - (rect.get_size().x % pattern->get_size().x)) : 0, valigned_bottom ? -(pattern->get_size().y - (rect.get_size().y % pattern->get_size().y)) : 0);

	Map<Vector2i, TileMapCell> output;
	if (!pattern->is_empty()) {
		if (!erase_button->is_pressed() && random_tile_checkbox->is_pressed()) {
			// Paint a random tile.
			for (int x = 0; x < rect.size.x; x++) {
				for (int y = 0; y < rect.size.y; y++) {
					Vector2i coords = rect.position + Vector2i(x, y);
					output.insert(coords, _pick_random_tile(pattern));
				}
			}
		} else {
			// Paint the pattern.
			TypedArray<Vector2i> used_cells = pattern->get_used_cells();
			for (int x = 0; x <= rect.size.x / pattern->get_size().x; x++) {
				for (int y = 0; y <= rect.size.y / pattern->get_size().y; y++) {
					Vector2i pattern_coords = rect.position + Vector2i(x, y) * pattern->get_size() + offset;
					for (int j = 0; j < used_cells.size(); j++) {
						Vector2i coords = pattern_coords + used_cells[j];
						if (rect.has_point(coords)) {
							output.insert(coords, TileMapCell(pattern->get_cell_source_id(used_cells[j]), pattern->get_cell_atlas_coords(used_cells[j]), pattern->get_cell_alternative_tile(used_cells[j])));
						}
					}
				}
			}
		}
	}

	return output;
}

Map<Vector2i, TileMapCell> TileMapEditorTilesPlugin::_draw_bucket_fill(Vector2i p_coords, bool p_contiguous) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, TileMapCell>();
	}

	if (tile_map_layer < 0) {
		return Map<Vector2i, TileMapCell>();
	}
	Map<Vector2i, TileMapCell> output;
	ERR_FAIL_INDEX_V(tile_map_layer, tile_map->get_layers_count(), output);

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, TileMapCell>();
	}

	// Get or create the pattern.
	TileMapPattern erase_pattern;
	erase_pattern.set_cell(Vector2i(0, 0), TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
	TileMapPattern *pattern = erase_button->is_pressed() ? &erase_pattern : selection_pattern;

	if (!pattern->is_empty()) {
		TileMapCell source = tile_map->get_cell(tile_map_layer, p_coords);

		// If we are filling empty tiles, compute the tilemap boundaries.
		Rect2i boundaries;
		if (source.source_id == TileSet::INVALID_SOURCE) {
			boundaries = tile_map->get_used_rect();
		}

		if (p_contiguous) {
			// Replace continuous tiles like the source.
			Set<Vector2i> already_checked;
			List<Vector2i> to_check;
			to_check.push_back(p_coords);
			while (!to_check.is_empty()) {
				Vector2i coords = to_check.back()->get();
				to_check.pop_back();
				if (!already_checked.has(coords)) {
					if (source.source_id == tile_map->get_cell_source_id(tile_map_layer, coords) &&
							source.get_atlas_coords() == tile_map->get_cell_atlas_coords(tile_map_layer, coords) &&
							source.alternative_tile == tile_map->get_cell_alternative_tile(tile_map_layer, coords) &&
							(source.source_id != TileSet::INVALID_SOURCE || boundaries.has_point(coords))) {
						if (!erase_button->is_pressed() && random_tile_checkbox->is_pressed()) {
							// Paint a random tile.
							output.insert(coords, _pick_random_tile(pattern));
						} else {
							// Paint the pattern.
							Vector2i pattern_coords = (coords - p_coords) % pattern->get_size(); // Note: it would be good to have posmodv for Vector2i.
							pattern_coords.x = pattern_coords.x < 0 ? pattern_coords.x + pattern->get_size().x : pattern_coords.x;
							pattern_coords.y = pattern_coords.y < 0 ? pattern_coords.y + pattern->get_size().y : pattern_coords.y;
							if (pattern->has_cell(pattern_coords)) {
								output.insert(coords, TileMapCell(pattern->get_cell_source_id(pattern_coords), pattern->get_cell_atlas_coords(pattern_coords), pattern->get_cell_alternative_tile(pattern_coords)));
							} else {
								output.insert(coords, TileMapCell());
							}
						}

						// Get surrounding tiles (handles different tile shapes).
						TypedArray<Vector2i> around = tile_map->get_surrounding_tiles(coords);
						for (int i = 0; i < around.size(); i++) {
							to_check.push_back(around[i]);
						}
					}
					already_checked.insert(coords);
				}
			}
		} else {
			// Replace all tiles like the source.
			TypedArray<Vector2i> to_check;
			if (source.source_id == TileSet::INVALID_SOURCE) {
				Rect2i rect = tile_map->get_used_rect();
				if (rect.size.x <= 0 || rect.size.y <= 0) {
					rect = Rect2i(p_coords, Vector2i(1, 1));
				}
				for (int x = boundaries.position.x; x < boundaries.get_end().x; x++) {
					for (int y = boundaries.position.y; y < boundaries.get_end().y; y++) {
						to_check.append(Vector2i(x, y));
					}
				}
			} else {
				to_check = tile_map->get_used_cells(tile_map_layer);
			}
			for (int i = 0; i < to_check.size(); i++) {
				Vector2i coords = to_check[i];
				if (source.source_id == tile_map->get_cell_source_id(tile_map_layer, coords) &&
						source.get_atlas_coords() == tile_map->get_cell_atlas_coords(tile_map_layer, coords) &&
						source.alternative_tile == tile_map->get_cell_alternative_tile(tile_map_layer, coords) &&
						(source.source_id != TileSet::INVALID_SOURCE || boundaries.has_point(coords))) {
					if (!erase_button->is_pressed() && random_tile_checkbox->is_pressed()) {
						// Paint a random tile.
						output.insert(coords, _pick_random_tile(pattern));
					} else {
						// Paint the pattern.
						Vector2i pattern_coords = (coords - p_coords) % pattern->get_size(); // Note: it would be good to have posmodv for Vector2i.
						pattern_coords.x = pattern_coords.x < 0 ? pattern_coords.x + pattern->get_size().x : pattern_coords.x;
						pattern_coords.y = pattern_coords.y < 0 ? pattern_coords.y + pattern->get_size().y : pattern_coords.y;
						if (pattern->has_cell(pattern_coords)) {
							output.insert(coords, TileMapCell(pattern->get_cell_source_id(pattern_coords), pattern->get_cell_atlas_coords(pattern_coords), pattern->get_cell_alternative_tile(pattern_coords)));
						} else {
							output.insert(coords, TileMapCell());
						}
					}
				}
			}
		}
	}
	return output;
}

void TileMapEditorTilesPlugin::_stop_dragging() {
	if (drag_type == DRAG_TYPE_NONE) {
		return;
	}

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	if (tile_map_layer < 0) {
		return;
	}
	ERR_FAIL_INDEX(tile_map_layer, tile_map->get_layers_count());

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Vector2 mpos = xform.affine_inverse().xform(CanvasItemEditor::get_singleton()->get_viewport_control()->get_local_mouse_position());

	switch (drag_type) {
		case DRAG_TYPE_SELECT: {
			undo_redo->create_action(TTR("Change selection"));
			undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());

			if (!Input::get_singleton()->is_key_pressed(KEY_SHIFT) && !Input::get_singleton()->is_key_pressed(KEY_CTRL)) {
				tile_map_selection.clear();
			}
			Rect2i rect = Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos) - tile_map->world_to_map(drag_start_mouse_pos)).abs();
			for (int x = rect.position.x; x <= rect.get_end().x; x++) {
				for (int y = rect.position.y; y <= rect.get_end().y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (Input::get_singleton()->is_key_pressed(KEY_CTRL)) {
						if (tile_map_selection.has(coords)) {
							tile_map_selection.erase(coords);
						}
					} else {
						if (tile_map->get_cell_source_id(tile_map_layer, coords) != TileSet::INVALID_SOURCE) {
							tile_map_selection.insert(coords);
						}
					}
				}
			}
			undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			undo_redo->commit_action(false);

			_update_selection_pattern_from_tilemap_selection();
			_update_tileset_selection_from_selection_pattern();
		} break;
		case DRAG_TYPE_MOVE: {
			Vector2i top_left;
			if (!tile_map_selection.is_empty()) {
				top_left = tile_map_selection.front()->get();
			}
			for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
				top_left = top_left.min(E->get());
			}

			Vector2i offset = drag_start_mouse_pos - tile_map->map_to_world(top_left);
			offset = tile_map->world_to_map(mpos - offset) - tile_map->world_to_map(drag_start_mouse_pos - offset);

			TypedArray<Vector2i> selection_used_cells = selection_pattern->get_used_cells();

			Vector2i coords;
			Map<Vector2i, TileMapCell> cells_undo;
			for (int i = 0; i < selection_used_cells.size(); i++) {
				coords = tile_map->map_pattern(top_left, selection_used_cells[i], selection_pattern);
				cells_undo[coords] = TileMapCell(drag_modified[coords].source_id, drag_modified[coords].get_atlas_coords(), drag_modified[coords].alternative_tile);
				coords = tile_map->map_pattern(top_left + offset, selection_used_cells[i], selection_pattern);
				cells_undo[coords] = TileMapCell(tile_map->get_cell_source_id(tile_map_layer, coords), tile_map->get_cell_atlas_coords(tile_map_layer, coords), tile_map->get_cell_alternative_tile(tile_map_layer, coords));
			}

			Map<Vector2i, TileMapCell> cells_do;
			for (int i = 0; i < selection_used_cells.size(); i++) {
				coords = tile_map->map_pattern(top_left, selection_used_cells[i], selection_pattern);
				cells_do[coords] = TileMapCell();
			}
			for (int i = 0; i < selection_used_cells.size(); i++) {
				coords = tile_map->map_pattern(top_left + offset, selection_used_cells[i], selection_pattern);
				cells_do[coords] = TileMapCell(selection_pattern->get_cell_source_id(selection_used_cells[i]), selection_pattern->get_cell_atlas_coords(selection_used_cells[i]), selection_pattern->get_cell_alternative_tile(selection_used_cells[i]));
			}
			undo_redo->create_action(TTR("Move tiles"));
			// Move the tiles.
			for (Map<Vector2i, TileMapCell>::Element *E = cells_do.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
			}
			for (Map<Vector2i, TileMapCell>::Element *E = cells_undo.front(); E; E = E->next()) {
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
			}

			// Update the selection.
			undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			tile_map_selection.clear();
			for (int i = 0; i < selection_used_cells.size(); i++) {
				coords = tile_map->map_pattern(top_left + offset, selection_used_cells[i], selection_pattern);
				tile_map_selection.insert(coords);
			}
			undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_PICK: {
			Rect2i rect = Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos) - tile_map->world_to_map(drag_start_mouse_pos)).abs();
			rect.size += Vector2i(1, 1);
			memdelete(selection_pattern);
			TypedArray<Vector2i> coords_array;
			for (int x = rect.position.x; x < rect.get_end().x; x++) {
				for (int y = rect.position.y; y < rect.get_end().y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (tile_map->get_cell_source_id(tile_map_layer, coords) != TileSet::INVALID_SOURCE) {
						coords_array.push_back(coords);
					}
				}
			}
			selection_pattern = tile_map->get_pattern(tile_map_layer, coords_array);
			if (!selection_pattern->is_empty()) {
				_update_tileset_selection_from_selection_pattern();
			} else {
				_update_selection_pattern_from_tileset_selection();
			}
			picker_button->set_pressed(false);
		} break;
		case DRAG_TYPE_PAINT: {
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, TileMapCell>::Element *E = drag_modified.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->key(), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
			}
			undo_redo->commit_action(false);
		} break;
		case DRAG_TYPE_LINE: {
			Map<Vector2i, TileMapCell> to_draw = _draw_line(drag_start_mouse_pos, drag_start_mouse_pos, mpos);
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, TileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
				if (!erase_button->is_pressed() && E->get().source_id == TileSet::INVALID_SOURCE) {
					continue;
				}
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->key(), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_RECT: {
			Map<Vector2i, TileMapCell> to_draw = _draw_rect(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos));
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, TileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
				if (!erase_button->is_pressed() && E->get().source_id == TileSet::INVALID_SOURCE) {
					continue;
				}
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->key(), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_BUCKET: {
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, TileMapCell>::Element *E = drag_modified.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->key(), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
			}
			undo_redo->commit_action(false);
		} break;
		case DRAG_TYPE_CLIPBOARD_PASTE: {
			Vector2 mouse_offset = (Vector2(tile_map_clipboard->get_size()) / 2.0 - Vector2(0.5, 0.5)) * tile_set->get_tile_size();
			undo_redo->create_action(TTR("Paste tiles"));
			TypedArray<Vector2i> used_cells = tile_map_clipboard->get_used_cells();
			for (int i = 0; i < used_cells.size(); i++) {
				Vector2i coords = tile_map->map_pattern(tile_map->world_to_map(mpos - mouse_offset), used_cells[i], tile_map_clipboard);
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, coords, tile_map_clipboard->get_cell_source_id(used_cells[i]), tile_map_clipboard->get_cell_atlas_coords(used_cells[i]), tile_map_clipboard->get_cell_alternative_tile(used_cells[i]));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, coords, tile_map->get_cell_source_id(tile_map_layer, coords), tile_map->get_cell_atlas_coords(tile_map_layer, coords), tile_map->get_cell_alternative_tile(tile_map_layer, coords));
			}
			undo_redo->commit_action();
		} break;
		default:
			break;
	}
	drag_type = DRAG_TYPE_NONE;
}

void TileMapEditorTilesPlugin::_update_fix_selected_and_hovered() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		hovered_tile.source_id = TileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		tile_set_selection.clear();
		tile_map_selection.clear();
		selection_pattern->clear();
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		hovered_tile.source_id = TileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		tile_set_selection.clear();
		tile_map_selection.clear();
		selection_pattern->clear();
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		hovered_tile.source_id = TileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		tile_set_selection.clear();
		tile_map_selection.clear();
		selection_pattern->clear();
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);

	// Clear hovered if needed.
	if (source_id != hovered_tile.source_id ||
			!tile_set->has_source(hovered_tile.source_id) ||
			!tile_set->get_source(hovered_tile.source_id)->has_tile(hovered_tile.get_atlas_coords()) ||
			!tile_set->get_source(hovered_tile.source_id)->has_alternative_tile(hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile)) {
		hovered_tile.source_id = TileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	}

	// Selection if needed.
	for (Set<TileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		const TileMapCell *selected = &(E->get());
		if (!tile_set->has_source(selected->source_id) ||
				!tile_set->get_source(selected->source_id)->has_tile(selected->get_atlas_coords()) ||
				!tile_set->get_source(selected->source_id)->has_alternative_tile(selected->get_atlas_coords(), selected->alternative_tile)) {
			tile_set_selection.erase(E);
		}
	}

	if (!tile_map_selection.is_empty()) {
		_update_selection_pattern_from_tilemap_selection();
	} else {
		_update_selection_pattern_from_tileset_selection();
	}
}

void TileMapEditorTilesPlugin::_update_selection_pattern_from_tilemap_selection() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	ERR_FAIL_INDEX(tile_map_layer, tile_map->get_layers_count());

	memdelete(selection_pattern);

	TypedArray<Vector2i> coords_array;
	for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
		coords_array.push_back(E->get());
	}
	selection_pattern = tile_map->get_pattern(tile_map_layer, coords_array);
}

void TileMapEditorTilesPlugin::_update_selection_pattern_from_tileset_selection() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Clear the tilemap selection.
	tile_map_selection.clear();

	// Clear the selected pattern.
	selection_pattern->clear();

	// Group per source.
	Map<int, List<const TileMapCell *>> per_source;
	for (Set<TileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		per_source[E->get().source_id].push_back(&(E->get()));
	}

	int vertical_offset = 0;
	for (Map<int, List<const TileMapCell *>>::Element *E_source = per_source.front(); E_source; E_source = E_source->next()) {
		// Per source.
		List<const TileMapCell *> unorganized;
		Rect2i encompassing_rect_coords;
		Map<Vector2i, const TileMapCell *> organized_pattern;

		TileSetSource *source = *tile_set->get_source(E_source->key());
		TileSetAtlasSource *atlas_source = Object::cast_to<TileSetAtlasSource>(source);
		if (atlas_source) {
			// Organize using coordinates.
			for (const TileMapCell *current : E_source->get()) {
				if (current->alternative_tile == 0) {
					organized_pattern[current->get_atlas_coords()] = current;
				} else {
					unorganized.push_back(current);
				}
			}

			// Compute the encompassing rect for the organized pattern.
			Map<Vector2i, const TileMapCell *>::Element *E_cell = organized_pattern.front();
			if (E_cell) {
				encompassing_rect_coords = Rect2i(E_cell->key(), Vector2i(1, 1));
				for (; E_cell; E_cell = E_cell->next()) {
					encompassing_rect_coords.expand_to(E_cell->key() + Vector2i(1, 1));
					encompassing_rect_coords.expand_to(E_cell->key());
				}
			}
		} else {
			// Add everything unorganized.
			for (const TileMapCell *cell : E_source->get()) {
				unorganized.push_back(cell);
			}
		}

		// Now add everything to the output pattern.
		for (Map<Vector2i, const TileMapCell *>::Element *E_cell = organized_pattern.front(); E_cell; E_cell = E_cell->next()) {
			selection_pattern->set_cell(E_cell->key() - encompassing_rect_coords.position + Vector2i(0, vertical_offset), E_cell->get()->source_id, E_cell->get()->get_atlas_coords(), E_cell->get()->alternative_tile);
		}
		Vector2i organized_size = selection_pattern->get_size();
		int unorganized_index = 0;
		for (const TileMapCell *cell : unorganized) {
			selection_pattern->set_cell(Vector2(organized_size.x + unorganized_index, vertical_offset), cell->source_id, cell->get_atlas_coords(), cell->alternative_tile);
			unorganized_index++;
		}
		vertical_offset += MAX(organized_size.y, 1);
	}
	CanvasItemEditor::get_singleton()->update_viewport();
}

void TileMapEditorTilesPlugin::_update_tileset_selection_from_selection_pattern() {
	tile_set_selection.clear();
	TypedArray<Vector2i> used_cells = selection_pattern->get_used_cells();
	for (int i = 0; i < used_cells.size(); i++) {
		Vector2i coords = used_cells[i];
		if (selection_pattern->get_cell_source_id(coords) != TileSet::INVALID_SOURCE) {
			tile_set_selection.insert(TileMapCell(selection_pattern->get_cell_source_id(coords), selection_pattern->get_cell_atlas_coords(coords), selection_pattern->get_cell_alternative_tile(coords)));
		}
	}
	_update_bottom_panel();
}

void TileMapEditorTilesPlugin::_tile_atlas_control_draw() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	TileSetAtlasSource *atlas = Object::cast_to<TileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Draw the selection.
	Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
	Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
	for (Set<TileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		if (E->get().source_id == source_id && E->get().alternative_tile == 0) {
			tile_atlas_control->draw_rect(atlas->get_tile_texture_region(E->get().get_atlas_coords()), selection_color, false);
		}
	}

	// Draw the hovered tile.
	if (hovered_tile.get_atlas_coords() != TileSetSource::INVALID_ATLAS_COORDS && hovered_tile.alternative_tile == 0 && !tile_set_dragging_selection) {
		tile_atlas_control->draw_rect(atlas->get_tile_texture_region(hovered_tile.get_atlas_coords()), Color(1.0, 1.0, 1.0), false);
	}

	// Draw the selection rect.
	if (tile_set_dragging_selection) {
		Vector2i start_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_set_drag_start_mouse_pos);
		Vector2i end_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_atlas_control->get_local_mouse_position());

		Rect2i region = Rect2i(start_tile, end_tile - start_tile).abs();
		region.size += Vector2i(1, 1);

		Set<Vector2i> to_draw;
		for (int x = region.position.x; x < region.get_end().x; x++) {
			for (int y = region.position.y; y < region.get_end().y; y++) {
				Vector2i tile = atlas->get_tile_at_coords(Vector2i(x, y));
				if (tile != TileSetSource::INVALID_ATLAS_COORDS) {
					to_draw.insert(tile);
				}
			}
		}
		Color selection_rect_color = selection_color.lightened(0.2);
		for (Set<Vector2i>::Element *E = to_draw.front(); E; E = E->next()) {
			tile_atlas_control->draw_rect(atlas->get_tile_texture_region(E->get()), selection_rect_color, false);
		}
	}
}

void TileMapEditorTilesPlugin::_tile_atlas_control_mouse_exited() {
	hovered_tile.source_id = TileSet::INVALID_SOURCE;
	hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	tile_set_dragging_selection = false;
	tile_atlas_control->update();
}

void TileMapEditorTilesPlugin::_tile_atlas_control_gui_input(const Ref<InputEvent> &p_event) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	TileSetAtlasSource *atlas = Object::cast_to<TileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Update the hovered tile
	hovered_tile.source_id = source_id;
	hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	Vector2i coords = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_atlas_control->get_local_mouse_position());
	if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
		coords = atlas->get_tile_at_coords(coords);
		if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
			hovered_tile.set_atlas_coords(coords);
			hovered_tile.alternative_tile = 0;
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		tile_atlas_control->update();
		alternative_tiles_control->update();
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
		if (mb->is_pressed()) { // Pressed
			tile_set_dragging_selection = true;
			tile_set_drag_start_mouse_pos = tile_atlas_control->get_local_mouse_position();
			if (!mb->is_shift_pressed()) {
				tile_set_selection.clear();
			}

			if (hovered_tile.get_atlas_coords() != TileSetSource::INVALID_ATLAS_COORDS && hovered_tile.alternative_tile == 0) {
				if (mb->is_shift_pressed() && tile_set_selection.has(TileMapCell(source_id, hovered_tile.get_atlas_coords(), 0))) {
					tile_set_selection.erase(TileMapCell(source_id, hovered_tile.get_atlas_coords(), 0));
				} else {
					tile_set_selection.insert(TileMapCell(source_id, hovered_tile.get_atlas_coords(), 0));
				}
			}
			_update_selection_pattern_from_tileset_selection();
		} else { // Released
			if (tile_set_dragging_selection) {
				if (!mb->is_shift_pressed()) {
					tile_set_selection.clear();
				}
				// Compute the covered area.
				Vector2i start_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_set_drag_start_mouse_pos);
				Vector2i end_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_atlas_control->get_local_mouse_position());
				if (start_tile != TileSetSource::INVALID_ATLAS_COORDS && end_tile != TileSetSource::INVALID_ATLAS_COORDS) {
					Rect2i region = Rect2i(start_tile, end_tile - start_tile).abs();
					region.size += Vector2i(1, 1);

					// To update the selection, we copy the selected/not selected status of the tiles we drag from.
					Vector2i start_coords = atlas->get_tile_at_coords(start_tile);
					if (mb->is_shift_pressed() && start_coords != TileSetSource::INVALID_ATLAS_COORDS && !tile_set_selection.has(TileMapCell(source_id, start_coords, 0))) {
						// Remove from the selection.
						for (int x = region.position.x; x < region.get_end().x; x++) {
							for (int y = region.position.y; y < region.get_end().y; y++) {
								Vector2i tile_coords = atlas->get_tile_at_coords(Vector2i(x, y));
								if (tile_coords != TileSetSource::INVALID_ATLAS_COORDS && tile_set_selection.has(TileMapCell(source_id, tile_coords, 0))) {
									tile_set_selection.erase(TileMapCell(source_id, tile_coords, 0));
								}
							}
						}
					} else {
						// Insert in the selection.
						for (int x = region.position.x; x < region.get_end().x; x++) {
							for (int y = region.position.y; y < region.get_end().y; y++) {
								Vector2i tile_coords = atlas->get_tile_at_coords(Vector2i(x, y));
								if (tile_coords != TileSetSource::INVALID_ATLAS_COORDS) {
									tile_set_selection.insert(TileMapCell(source_id, tile_coords, 0));
								}
							}
						}
					}
				}
				_update_selection_pattern_from_tileset_selection();
			}
			tile_set_dragging_selection = false;
		}
		tile_atlas_control->update();
	}
}

void TileMapEditorTilesPlugin::_tile_alternatives_control_draw() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	TileSetAtlasSource *atlas = Object::cast_to<TileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Draw the selection.
	for (Set<TileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		if (E->get().source_id == source_id && E->get().get_atlas_coords() != TileSetSource::INVALID_ATLAS_COORDS && E->get().alternative_tile > 0) {
			Rect2i rect = tile_atlas_view->get_alternative_tile_rect(E->get().get_atlas_coords(), E->get().alternative_tile);
			if (rect != Rect2i()) {
				alternative_tiles_control->draw_rect(rect, Color(0.2, 0.2, 1.0), false);
			}
		}
	}

	// Draw hovered tile.
	if (hovered_tile.get_atlas_coords() != TileSetSource::INVALID_ATLAS_COORDS && hovered_tile.alternative_tile > 0) {
		Rect2i rect = tile_atlas_view->get_alternative_tile_rect(hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile);
		if (rect != Rect2i()) {
			alternative_tiles_control->draw_rect(rect, Color(1.0, 1.0, 1.0), false);
		}
	}
}

void TileMapEditorTilesPlugin::_tile_alternatives_control_mouse_exited() {
	hovered_tile.source_id = TileSet::INVALID_SOURCE;
	hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	tile_set_dragging_selection = false;
	alternative_tiles_control->update();
}

void TileMapEditorTilesPlugin::_tile_alternatives_control_gui_input(const Ref<InputEvent> &p_event) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	TileSetAtlasSource *atlas = Object::cast_to<TileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Update the hovered tile
	hovered_tile.source_id = source_id;
	hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	Vector3i alternative_coords = tile_atlas_view->get_alternative_tile_at_pos(alternative_tiles_control->get_local_mouse_position());
	Vector2i coords = Vector2i(alternative_coords.x, alternative_coords.y);
	int alternative = alternative_coords.z;
	if (coords != TileSetSource::INVALID_ATLAS_COORDS && alternative != TileSetSource::INVALID_TILE_ALTERNATIVE) {
		hovered_tile.set_atlas_coords(coords);
		hovered_tile.alternative_tile = alternative;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		tile_atlas_control->update();
		alternative_tiles_control->update();
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
		if (mb->is_pressed()) { // Pressed
			// Left click pressed.
			if (!mb->is_shift_pressed()) {
				tile_set_selection.clear();
			}

			if (coords != TileSetSource::INVALID_ATLAS_COORDS && alternative != TileSetAtlasSource::INVALID_TILE_ALTERNATIVE) {
				if (mb->is_shift_pressed() && tile_set_selection.has(TileMapCell(source_id, hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile))) {
					tile_set_selection.erase(TileMapCell(source_id, hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile));
				} else {
					tile_set_selection.insert(TileMapCell(source_id, hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile));
				}
			}
			_update_selection_pattern_from_tileset_selection();
		}
		tile_atlas_control->update();
		alternative_tiles_control->update();
	}
}

void TileMapEditorTilesPlugin::_set_tile_map_selection(const TypedArray<Vector2i> &p_selection) {
	tile_map_selection.clear();
	for (int i = 0; i < p_selection.size(); i++) {
		tile_map_selection.insert(p_selection[i]);
	}
	_update_selection_pattern_from_tilemap_selection();
	_update_tileset_selection_from_selection_pattern();
	CanvasItemEditor::get_singleton()->update_viewport();
}

TypedArray<Vector2i> TileMapEditorTilesPlugin::_get_tile_map_selection() const {
	TypedArray<Vector2i> output;
	for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
		output.push_back(E->get());
	}
	return output;
}

void TileMapEditorTilesPlugin::edit(ObjectID p_tile_map_id, int p_tile_map_layer) {
	_stop_dragging(); // Avoids staying in a wrong drag state.

	if (tile_map_id != p_tile_map_id) {
		tile_map_id = p_tile_map_id;

		// Clear the selection.
		tile_set_selection.clear();
		tile_map_selection.clear();
		selection_pattern->clear();
	}

	tile_map_layer = p_tile_map_layer;
}

void TileMapEditorTilesPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_scene_thumbnail_done"), &TileMapEditorTilesPlugin::_scene_thumbnail_done);
	ClassDB::bind_method(D_METHOD("_set_tile_map_selection", "selection"), &TileMapEditorTilesPlugin::_set_tile_map_selection);
	ClassDB::bind_method(D_METHOD("_get_tile_map_selection"), &TileMapEditorTilesPlugin::_get_tile_map_selection);
}

TileMapEditorTilesPlugin::TileMapEditorTilesPlugin() {
	CanvasItemEditor::get_singleton()->get_viewport_control()->connect("mouse_exited", callable_mp(this, &TileMapEditorTilesPlugin::_mouse_exited_viewport));

	// --- Shortcuts ---
	ED_SHORTCUT("tiles_editor/cut", TTR("Cut"), KEY_MASK_CMD | KEY_X);
	ED_SHORTCUT("tiles_editor/copy", TTR("Copy"), KEY_MASK_CMD | KEY_C);
	ED_SHORTCUT("tiles_editor/paste", TTR("Paste"), KEY_MASK_CMD | KEY_V);
	ED_SHORTCUT("tiles_editor/cancel", TTR("Cancel"), KEY_ESCAPE);
	ED_SHORTCUT("tiles_editor/delete", TTR("Delete"), KEY_DELETE);

	// --- Toolbar ---
	toolbar = memnew(HBoxContainer);
	toolbar->set_h_size_flags(SIZE_EXPAND_FILL);

	HBoxContainer *tilemap_tiles_tools_buttons = memnew(HBoxContainer);

	tool_buttons_group.instantiate();

	select_tool_button = memnew(Button);
	select_tool_button->set_flat(true);
	select_tool_button->set_toggle_mode(true);
	select_tool_button->set_button_group(tool_buttons_group);
	select_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/selection_tool", "Selection", KEY_S));
	select_tool_button->connect("pressed", callable_mp(this, &TileMapEditorTilesPlugin::_update_toolbar));
	tilemap_tiles_tools_buttons->add_child(select_tool_button);

	paint_tool_button = memnew(Button);
	paint_tool_button->set_flat(true);
	paint_tool_button->set_toggle_mode(true);
	paint_tool_button->set_button_group(tool_buttons_group);
	paint_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/paint_tool", "Paint", KEY_D));
	paint_tool_button->connect("pressed", callable_mp(this, &TileMapEditorTilesPlugin::_update_toolbar));
	tilemap_tiles_tools_buttons->add_child(paint_tool_button);

	line_tool_button = memnew(Button);
	line_tool_button->set_flat(true);
	line_tool_button->set_toggle_mode(true);
	line_tool_button->set_button_group(tool_buttons_group);
	line_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/line_tool", "Line", KEY_L));
	line_tool_button->connect("pressed", callable_mp(this, &TileMapEditorTilesPlugin::_update_toolbar));
	tilemap_tiles_tools_buttons->add_child(line_tool_button);

	rect_tool_button = memnew(Button);
	rect_tool_button->set_flat(true);
	rect_tool_button->set_toggle_mode(true);
	rect_tool_button->set_button_group(tool_buttons_group);
	rect_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/rect_tool", "Rect", KEY_R));
	rect_tool_button->connect("pressed", callable_mp(this, &TileMapEditorTilesPlugin::_update_toolbar));
	tilemap_tiles_tools_buttons->add_child(rect_tool_button);

	bucket_tool_button = memnew(Button);
	bucket_tool_button->set_flat(true);
	bucket_tool_button->set_toggle_mode(true);
	bucket_tool_button->set_button_group(tool_buttons_group);
	bucket_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/bucket_tool", "Bucket", KEY_B));
	bucket_tool_button->connect("pressed", callable_mp(this, &TileMapEditorTilesPlugin::_update_toolbar));
	tilemap_tiles_tools_buttons->add_child(bucket_tool_button);
	toolbar->add_child(tilemap_tiles_tools_buttons);

	// -- TileMap tool settings --
	tools_settings = memnew(HBoxContainer);
	toolbar->add_child(tools_settings);

	tools_settings_vsep = memnew(VSeparator);
	tools_settings->add_child(tools_settings_vsep);

	// Picker
	picker_button = memnew(Button);
	picker_button->set_flat(true);
	picker_button->set_toggle_mode(true);
	picker_button->set_shortcut(ED_SHORTCUT("tiles_editor/picker", "Picker", KEY_P));
	picker_button->connect("pressed", callable_mp(CanvasItemEditor::get_singleton(), &CanvasItemEditor::update_viewport));
	tools_settings->add_child(picker_button);

	// Erase button.
	erase_button = memnew(Button);
	erase_button->set_flat(true);
	erase_button->set_toggle_mode(true);
	erase_button->set_shortcut(ED_SHORTCUT("tiles_editor/eraser", "Eraser", KEY_E));
	erase_button->connect("pressed", callable_mp(CanvasItemEditor::get_singleton(), &CanvasItemEditor::update_viewport));
	tools_settings->add_child(erase_button);

	// Separator 2.
	tools_settings_vsep_2 = memnew(VSeparator);
	tools_settings->add_child(tools_settings_vsep_2);

	// Continuous checkbox.
	bucket_continuous_checkbox = memnew(CheckBox);
	bucket_continuous_checkbox->set_flat(true);
	bucket_continuous_checkbox->set_text(TTR("Contiguous"));
	tools_settings->add_child(bucket_continuous_checkbox);

	// Random tile checkbox.
	random_tile_checkbox = memnew(CheckBox);
	random_tile_checkbox->set_flat(true);
	random_tile_checkbox->set_text(TTR("Place Random Tile"));
	random_tile_checkbox->connect("toggled", callable_mp(this, &TileMapEditorTilesPlugin::_on_random_tile_checkbox_toggled));
	tools_settings->add_child(random_tile_checkbox);

	// Random tile scattering.
	scatter_label = memnew(Label);
	scatter_label->set_tooltip(TTR("Defines the probability of painting nothing instead of a randomly selected tile."));
	scatter_label->set_text(TTR("Scattering:"));
	tools_settings->add_child(scatter_label);

	scatter_spinbox = memnew(SpinBox);
	scatter_spinbox->set_min(0.0);
	scatter_spinbox->set_max(1000);
	scatter_spinbox->set_step(0.001);
	scatter_spinbox->set_tooltip(TTR("Defines the probability of painting nothing instead of a randomly selected tile."));
	scatter_spinbox->get_line_edit()->add_theme_constant_override("minimum_character_width", 4);
	scatter_spinbox->connect("value_changed", callable_mp(this, &TileMapEditorTilesPlugin::_on_scattering_spinbox_changed));
	tools_settings->add_child(scatter_spinbox);

	_on_random_tile_checkbox_toggled(false);

	// Default tool.
	paint_tool_button->set_pressed(true);
	_update_toolbar();

	// --- Bottom panel ---
	set_name("Tiles");

	missing_source_label = memnew(Label);
	missing_source_label->set_text(TTR("This TileMap's TileSet has no source configured. Edit the TileSet resource to add one."));
	missing_source_label->set_h_size_flags(SIZE_EXPAND_FILL);
	missing_source_label->set_v_size_flags(SIZE_EXPAND_FILL);
	missing_source_label->set_align(Label::ALIGN_CENTER);
	missing_source_label->set_valign(Label::VALIGN_CENTER);
	missing_source_label->hide();
	add_child(missing_source_label);

	atlas_sources_split_container = memnew(HSplitContainer);
	atlas_sources_split_container->set_h_size_flags(SIZE_EXPAND_FILL);
	atlas_sources_split_container->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(atlas_sources_split_container);

	sources_list = memnew(ItemList);
	sources_list->set_fixed_icon_size(Size2i(60, 60) * EDSCALE);
	sources_list->set_h_size_flags(SIZE_EXPAND_FILL);
	sources_list->set_stretch_ratio(0.25);
	sources_list->set_custom_minimum_size(Size2i(70, 0) * EDSCALE);
	sources_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	sources_list->connect("item_selected", callable_mp(this, &TileMapEditorTilesPlugin::_update_fix_selected_and_hovered).unbind(1));
	sources_list->connect("item_selected", callable_mp(this, &TileMapEditorTilesPlugin::_update_bottom_panel).unbind(1));
	sources_list->connect("item_selected", callable_mp(TilesEditor::get_singleton(), &TilesEditor::set_sources_lists_current));
	sources_list->connect("visibility_changed", callable_mp(TilesEditor::get_singleton(), &TilesEditor::synchronize_sources_list), varray(sources_list));
	atlas_sources_split_container->add_child(sources_list);

	// Tile atlas source.
	tile_atlas_view = memnew(TileAtlasView);
	tile_atlas_view->set_h_size_flags(SIZE_EXPAND_FILL);
	tile_atlas_view->set_v_size_flags(SIZE_EXPAND_FILL);
	tile_atlas_view->set_texture_grid_visible(false);
	tile_atlas_view->set_tile_shape_grid_visible(false);
	tile_atlas_view->connect("transform_changed", callable_mp(TilesEditor::get_singleton(), &TilesEditor::set_atlas_view_transform));
	atlas_sources_split_container->add_child(tile_atlas_view);

	tile_atlas_control = memnew(Control);
	tile_atlas_control->connect("draw", callable_mp(this, &TileMapEditorTilesPlugin::_tile_atlas_control_draw));
	tile_atlas_control->connect("mouse_exited", callable_mp(this, &TileMapEditorTilesPlugin::_tile_atlas_control_mouse_exited));
	tile_atlas_control->connect("gui_input", callable_mp(this, &TileMapEditorTilesPlugin::_tile_atlas_control_gui_input));
	tile_atlas_view->add_control_over_atlas_tiles(tile_atlas_control);

	alternative_tiles_control = memnew(Control);
	alternative_tiles_control->connect("draw", callable_mp(this, &TileMapEditorTilesPlugin::_tile_alternatives_control_draw));
	alternative_tiles_control->connect("mouse_exited", callable_mp(this, &TileMapEditorTilesPlugin::_tile_alternatives_control_mouse_exited));
	alternative_tiles_control->connect("gui_input", callable_mp(this, &TileMapEditorTilesPlugin::_tile_alternatives_control_gui_input));
	tile_atlas_view->add_control_over_alternative_tiles(alternative_tiles_control);

	// Scenes collection source.
	scene_tiles_list = memnew(ItemList);
	scene_tiles_list->set_h_size_flags(SIZE_EXPAND_FILL);
	scene_tiles_list->set_v_size_flags(SIZE_EXPAND_FILL);
	scene_tiles_list->set_drag_forwarding(this);
	scene_tiles_list->set_select_mode(ItemList::SELECT_MULTI);
	scene_tiles_list->connect("multi_selected", callable_mp(this, &TileMapEditorTilesPlugin::_scenes_list_multi_selected));
	scene_tiles_list->connect("nothing_selected", callable_mp(this, &TileMapEditorTilesPlugin::_scenes_list_nothing_selected));
	scene_tiles_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	atlas_sources_split_container->add_child(scene_tiles_list);

	// Invalid source label.
	invalid_source_label = memnew(Label);
	invalid_source_label->set_text(TTR("Invalid source selected."));
	invalid_source_label->set_h_size_flags(SIZE_EXPAND_FILL);
	invalid_source_label->set_v_size_flags(SIZE_EXPAND_FILL);
	invalid_source_label->set_align(Label::ALIGN_CENTER);
	invalid_source_label->set_valign(Label::VALIGN_CENTER);
	invalid_source_label->hide();
	atlas_sources_split_container->add_child(invalid_source_label);

	_update_bottom_panel();
}

TileMapEditorTilesPlugin::~TileMapEditorTilesPlugin() {
	memdelete(selection_pattern);
	memdelete(tile_map_clipboard);
}

void TileMapEditorTerrainsPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED:
			paint_tool_button->set_icon(get_theme_icon(SNAME("Edit"), SNAME("EditorIcons")));
			picker_button->set_icon(get_theme_icon(SNAME("ColorPick"), SNAME("EditorIcons")));
			erase_button->set_icon(get_theme_icon(SNAME("Eraser"), SNAME("EditorIcons")));
			break;
	}
}

void TileMapEditorTerrainsPlugin::tile_set_changed() {
	_update_terrains_cache();
	_update_terrains_tree();
	_update_tiles_list();
}

void TileMapEditorTerrainsPlugin::_update_toolbar() {
	// Hide all settings.
	for (int i = 0; i < tools_settings->get_child_count(); i++) {
		Object::cast_to<CanvasItem>(tools_settings->get_child(i))->hide();
	}

	// Show only the correct settings.
	if (tool_buttons_group->get_pressed_button() == paint_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
	}
}

Control *TileMapEditorTerrainsPlugin::get_toolbar() const {
	return toolbar;
}

Map<Vector2i, TileSet::CellNeighbor> TileMapEditorTerrainsPlugin::Constraint::get_overlapping_coords_and_peering_bits() const {
	Map<Vector2i, TileSet::CellNeighbor> output;
	Ref<TileSet> tile_set = tile_map->get_tileset();
	ERR_FAIL_COND_V(!tile_set.is_valid(), output);

	TileSet::TileShape shape = tile_set->get_tile_shape();
	if (shape == TileSet::TILE_SHAPE_SQUARE) {
		switch (bit) {
			case 0:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_SIDE;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_SIDE;
				break;
			case 1:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;
				break;
			case 2:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_SIDE;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_SIDE;
				break;
			case 3:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
				break;
			default:
				ERR_FAIL_V(output);
		}
	} else if (shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		switch (bit) {
			case 0:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_RIGHT_CORNER)] = TileSet::CELL_NEIGHBOR_LEFT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_CORNER;
				break;
			case 1:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
				break;
			case 2:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_CORNER)] = TileSet::CELL_NEIGHBOR_TOP_CORNER;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_RIGHT_CORNER;
				break;
			case 3:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
				output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
				break;
			default:
				ERR_FAIL_V(output);
		}
	} else {
		// Half offset shapes.
		TileSet::TileOffsetAxis offset_axis = tile_set->get_tile_offset_axis();
		if (offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (bit) {
				case 0:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_SIDE;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_SIDE;
					break;
				case 1:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_CORNER;
					break;
				case 2:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
					break;
				case 3:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;
					break;
				case 4:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
					break;
				default:
					ERR_FAIL_V(output);
			}
		} else {
			switch (bit) {
				case 0:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
					break;
				case 1:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
					break;
				case 2:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_CORNER;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
					break;
				case 3:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_SIDE;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_SIDE;
					break;
				case 4:
					output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
					output[tile_map->get_neighbor_cell(base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
					break;
				default:
					ERR_FAIL_V(output);
			}
		}
	}
	return output;
}

TileMapEditorTerrainsPlugin::Constraint::Constraint(const TileMap *p_tile_map, const Vector2i &p_position, const TileSet::CellNeighbor &p_bit, int p_terrain) {
	// The way we build the constraint make it easy to detect conflicting constraints.
	tile_map = p_tile_map;

	Ref<TileSet> tile_set = tile_map->get_tileset();
	ERR_FAIL_COND(!tile_set.is_valid());

	TileSet::TileShape shape = tile_set->get_tile_shape();
	if (shape == TileSet::TILE_SHAPE_SQUARE || shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		switch (p_bit) {
			case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
				bit = 0;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
				bit = 1;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
				bit = 2;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
				bit = 3;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
				bit = 0;
				base_cell_coords = p_tile_map->get_neighbor_cell(p_position, p_bit);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
				bit = 1;
				base_cell_coords = p_tile_map->get_neighbor_cell(p_position, p_bit);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			case TileSet::CELL_NEIGHBOR_TOP_CORNER:
				bit = 2;
				base_cell_coords = p_tile_map->get_neighbor_cell(p_position, p_bit);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
				bit = 3;
				base_cell_coords = p_tile_map->get_neighbor_cell(p_position, p_bit);
				break;
			default:
				ERR_FAIL();
				break;
		}
	} else {
		// Half-offset shapes
		TileSet::TileOffsetAxis offset_axis = tile_set->get_tile_offset_axis();
		if (offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (p_bit) {
				case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
					bit = 0;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
					bit = 1;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
					bit = 2;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
					bit = 3;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
					bit = 4;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
					bit = 1;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
					bit = 0;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
					bit = 3;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
					bit = 2;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_CORNER:
					bit = 1;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
					bit = 4;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
					bit = 3;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
					break;
				default:
					ERR_FAIL();
					break;
			}
		} else {
			switch (p_bit) {
				case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
					bit = 0;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
					bit = 1;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
					bit = 2;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
					bit = 3;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
					bit = 0;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
					bit = 4;
					base_cell_coords = p_position;
					break;
				case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
					bit = 2;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
					bit = 1;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
					bit = 0;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_SIDE:
					bit = 3;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
					bit = 2;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
					break;
				case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
					bit = 4;
					base_cell_coords = p_tile_map->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
					break;
				default:
					ERR_FAIL();
					break;
			}
		}
	}
	terrain = p_terrain;
}

Set<TileMapEditorTerrainsPlugin::TerrainsTilePattern> TileMapEditorTerrainsPlugin::_get_valid_terrains_tile_patterns_for_constraints(int p_terrain_set, const Vector2i &p_position, Set<Constraint> p_constraints) const {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Set<TerrainsTilePattern>();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Set<TerrainsTilePattern>();
	}

	// Returns all tiles compatible with the given constraints.
	Set<TerrainsTilePattern> compatible_terrain_tile_patterns;
	for (Map<TerrainsTilePattern, Set<TileMapCell>>::Element *E = per_terrain_terrains_tile_patterns_tiles[p_terrain_set].front(); E; E = E->next()) {
		int valid = true;
		int in_pattern_count = 0;
		for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(i);
			if (tile_set->is_valid_peering_bit_terrain(p_terrain_set, bit)) {
				// Check if the bit is compatible with the constraints.
				Constraint terrain_bit_constraint = Constraint(tile_map, p_position, bit, E->key()[in_pattern_count]);

				Set<Constraint>::Element *in_set_constraint_element = p_constraints.find(terrain_bit_constraint);
				if (in_set_constraint_element && in_set_constraint_element->get().get_terrain() != terrain_bit_constraint.get_terrain()) {
					valid = false;
					break;
				}
				in_pattern_count++;
			}
		}

		if (valid) {
			compatible_terrain_tile_patterns.insert(E->key());
		}
	}

	return compatible_terrain_tile_patterns;
}

Set<TileMapEditorTerrainsPlugin::Constraint> TileMapEditorTerrainsPlugin::_get_constraints_from_removed_cells_list(const Set<Vector2i> &p_to_replace, int p_terrain_set) const {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Set<Constraint>();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Set<Constraint>();
	}

	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(), Set<Constraint>());
	ERR_FAIL_INDEX_V(tile_map_layer, tile_map->get_layers_count(), Set<Constraint>());

	// Build a set of dummy constraints get the constrained points.
	Set<Constraint> dummy_constraints;
	for (Set<Vector2i>::Element *E = p_to_replace.front(); E; E = E->next()) {
		for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) { // Iterates over sides.
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(i);
			if (tile_set->is_valid_peering_bit_terrain(p_terrain_set, bit)) {
				dummy_constraints.insert(Constraint(tile_map, E->get(), bit, -1));
			}
		}
	}

	// For each constrained point, we get all overlapping tiles, and select the most adequate terrain for it.
	Set<Constraint> constraints;
	for (Set<Constraint>::Element *E = dummy_constraints.front(); E; E = E->next()) {
		Constraint c = E->get();

		Map<int, int> terrain_count;

		// Count the number of occurrences per terrain.
		Map<Vector2i, TileSet::CellNeighbor> overlapping_terrain_bits = c.get_overlapping_coords_and_peering_bits();
		for (Map<Vector2i, TileSet::CellNeighbor>::Element *E_overlapping = overlapping_terrain_bits.front(); E_overlapping; E_overlapping = E_overlapping->next()) {
			if (!p_to_replace.has(E_overlapping->key())) {
				TileMapCell neighbor_cell = tile_map->get_cell(tile_map_layer, E_overlapping->key());
				TileData *neighbor_tile_data = nullptr;
				if (terrain_tiles.has(neighbor_cell) && terrain_tiles[neighbor_cell]->get_terrain_set() == p_terrain_set) {
					neighbor_tile_data = terrain_tiles[neighbor_cell];
				}

				int terrain = neighbor_tile_data ? neighbor_tile_data->get_peering_bit_terrain(TileSet::CellNeighbor(E_overlapping->get())) : -1;
				if (terrain_count.has(terrain)) {
					terrain_count[terrain] = 0;
				}
				terrain_count[terrain] += 1;
			}
		}

		// Get the terrain with the max number of occurrences.
		int max = 0;
		int max_terrain = -1;
		for (Map<int, int>::Element *E_terrain_count = terrain_count.front(); E_terrain_count; E_terrain_count = E_terrain_count->next()) {
			if (E_terrain_count->get() > max) {
				max = E_terrain_count->get();
				max_terrain = E_terrain_count->key();
			}
		}

		// Set the adequate terrain.
		if (max > 0) {
			c.set_terrain(max_terrain);
			constraints.insert(c);
		}
	}

	return constraints;
}

Set<TileMapEditorTerrainsPlugin::Constraint> TileMapEditorTerrainsPlugin::_get_constraints_from_added_tile(Vector2i p_position, int p_terrain_set, TerrainsTilePattern p_terrains_tile_pattern) const {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Set<TileMapEditorTerrainsPlugin::Constraint>();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Set<TileMapEditorTerrainsPlugin::Constraint>();
	}

	// Compute the constraints needed from the surrounding tiles.
	Set<TileMapEditorTerrainsPlugin::Constraint> output;
	int in_pattern_count = 0;
	for (uint32_t i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		TileSet::CellNeighbor side = TileSet::CellNeighbor(i);
		if (tile_set->is_valid_peering_bit_terrain(p_terrain_set, side)) {
			Constraint c = Constraint(tile_map, p_position, side, p_terrains_tile_pattern[in_pattern_count]);
			output.insert(c);
			in_pattern_count++;
		}
	}

	return output;
}

Map<Vector2i, TileMapEditorTerrainsPlugin::TerrainsTilePattern> TileMapEditorTerrainsPlugin::_wave_function_collapse(const Set<Vector2i> &p_to_replace, int p_terrain_set, const Set<TileMapEditorTerrainsPlugin::Constraint> p_constraints) const {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, TerrainsTilePattern>();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, TileMapEditorTerrainsPlugin::TerrainsTilePattern>();
	}

	// Copy the constraints set.
	Set<TileMapEditorTerrainsPlugin::Constraint> constraints = p_constraints;

	// Compute all acceptable tiles for each cell.
	Map<Vector2i, Set<TerrainsTilePattern>> per_cell_acceptable_tiles;
	for (Set<Vector2i>::Element *E = p_to_replace.front(); E; E = E->next()) {
		per_cell_acceptable_tiles[E->get()] = _get_valid_terrains_tile_patterns_for_constraints(p_terrain_set, E->get(), constraints);
	}

	// Output map.
	Map<Vector2i, TerrainsTilePattern> output;

	// Add all positions to a set.
	Set<Vector2i> to_replace = Set<Vector2i>(p_to_replace);
	while (!to_replace.is_empty()) {
		// Compute the minimum number of tile possibilities for each cell.
		int min_nb_possibilities = 100000000;
		for (Map<Vector2i, Set<TerrainsTilePattern>>::Element *E = per_cell_acceptable_tiles.front(); E; E = E->next()) {
			min_nb_possibilities = MIN(min_nb_possibilities, E->get().size());
		}

		// Get the set of possible cells to fill.
		LocalVector<Vector2i> to_choose_from;
		for (Map<Vector2i, Set<TerrainsTilePattern>>::Element *E = per_cell_acceptable_tiles.front(); E; E = E->next()) {
			if (E->get().size() == min_nb_possibilities) {
				to_choose_from.push_back(E->key());
			}
		}

		// Randomly pick a tile out of the most constrained.
		Vector2i selected_cell_to_replace = to_choose_from[Math::random(0, to_choose_from.size() - 1)];

		// Randomly select a tile out of them the put it in the grid.
		Set<TerrainsTilePattern> valid_tiles = per_cell_acceptable_tiles[selected_cell_to_replace];
		if (valid_tiles.is_empty()) {
			// No possibilities :/
			break;
		}
		int random_terrain_tile_pattern_index = Math::random(0, valid_tiles.size() - 1);
		Set<TerrainsTilePattern>::Element *E = valid_tiles.front();
		for (int i = 0; i < random_terrain_tile_pattern_index; i++) {
			E = E->next();
		}
		TerrainsTilePattern selected_terrain_tile_pattern = E->get();

		// Set the selected cell into the output.
		output[selected_cell_to_replace] = selected_terrain_tile_pattern;
		to_replace.erase(selected_cell_to_replace);
		per_cell_acceptable_tiles.erase(selected_cell_to_replace);

		// Add the new constraints from the added tiles.
		Set<TileMapEditorTerrainsPlugin::Constraint> new_constraints = _get_constraints_from_added_tile(selected_cell_to_replace, p_terrain_set, selected_terrain_tile_pattern);
		for (Set<TileMapEditorTerrainsPlugin::Constraint>::Element *E_constraint = new_constraints.front(); E_constraint; E_constraint = E_constraint->next()) {
			constraints.insert(E_constraint->get());
		}

		// Compute valid tiles again for neighbors.
		for (uint32_t i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
			TileSet::CellNeighbor side = TileSet::CellNeighbor(i);
			if (tile_map->is_existing_neighbor(side)) {
				Vector2i neighbor = tile_map->get_neighbor_cell(selected_cell_to_replace, side);
				if (to_replace.has(neighbor)) {
					per_cell_acceptable_tiles[neighbor] = _get_valid_terrains_tile_patterns_for_constraints(p_terrain_set, neighbor, constraints);
				}
			}
		}
	}
	return output;
}

TileMapCell TileMapEditorTerrainsPlugin::_get_random_tile_from_pattern(int p_terrain_set, TerrainsTilePattern p_terrain_tile_pattern) const {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return TileMapCell();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return TileMapCell();
	}

	// Count the sum of probabilities.
	double sum = 0.0;
	Set<TileMapCell> set = per_terrain_terrains_tile_patterns_tiles[p_terrain_set][p_terrain_tile_pattern];
	for (Set<TileMapCell>::Element *E = set.front(); E; E = E->next()) {
		if (E->get().source_id >= 0) {
			Ref<TileSetSource> source = tile_set->get_source(E->get().source_id);

			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				TileData *tile_data = Object::cast_to<TileData>(atlas_source->get_tile_data(E->get().get_atlas_coords(), E->get().alternative_tile));
				sum += tile_data->get_probability();
			} else {
				sum += 1.0;
			}
		} else {
			sum += 1.0;
		}
	}

	// Generate a random number.
	double count = 0.0;
	double picked = Math::random(0.0, sum);

	// Pick the tile.
	for (Set<TileMapCell>::Element *E = set.front(); E; E = E->next()) {
		if (E->get().source_id >= 0) {
			Ref<TileSetSource> source = tile_set->get_source(E->get().source_id);

			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				TileData *tile_data = Object::cast_to<TileData>(atlas_source->get_tile_data(E->get().get_atlas_coords(), E->get().alternative_tile));
				count += tile_data->get_probability();
			} else {
				count += 1.0;
			}
		} else {
			count += 1.0;
		}

		if (count >= picked) {
			return E->get();
		}
	}

	ERR_FAIL_V(TileMapCell());
}

Map<Vector2i, TileMapCell> TileMapEditorTerrainsPlugin::_draw_terrains(const Map<Vector2i, TerrainsTilePattern> &p_to_paint, int p_terrain_set) const {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, TileMapCell>();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, TileMapCell>();
	}

	Map<Vector2i, TileMapCell> output;

	// Add the constraints from the added tiles.
	Set<TileMapEditorTerrainsPlugin::Constraint> added_tiles_constraints_set;
	for (Map<Vector2i, TerrainsTilePattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		Vector2i coords = E_to_paint->key();
		TerrainsTilePattern terrains_tile_pattern = E_to_paint->get();

		Set<TileMapEditorTerrainsPlugin::Constraint> cell_constraints = _get_constraints_from_added_tile(coords, p_terrain_set, terrains_tile_pattern);
		for (Set<TileMapEditorTerrainsPlugin::Constraint>::Element *E = cell_constraints.front(); E; E = E->next()) {
			added_tiles_constraints_set.insert(E->get());
		}
	}

	// Build the list of potential tiles to replace.
	Set<Vector2i> potential_to_replace;
	for (Map<Vector2i, TerrainsTilePattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		Vector2i coords = E_to_paint->key();
		for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
			if (tile_map->is_existing_neighbor(TileSet::CellNeighbor(i))) {
				Vector2i neighbor = tile_map->get_neighbor_cell(coords, TileSet::CellNeighbor(i));
				if (!p_to_paint.has(neighbor)) {
					potential_to_replace.insert(neighbor);
				}
			}
		}
	}

	// Set of tiles to replace
	Set<Vector2i> to_replace;

	// Add the central tiles to the one to replace. TODO: maybe change that.
	for (Map<Vector2i, TerrainsTilePattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		to_replace.insert(E_to_paint->key());
	}

	// Add the constraints from the surroundings of the modified areas.
	Set<TileMapEditorTerrainsPlugin::Constraint> removed_cells_constraints_set;
	bool to_replace_modified = true;
	while (to_replace_modified) {
		// Get the constraints from the removed cells.
		removed_cells_constraints_set = _get_constraints_from_removed_cells_list(to_replace, p_terrain_set);

		// Filter the sources to make sure they are in the potential_to_replace.
		Map<Constraint, Set<Vector2i>> source_tiles_of_constraint;
		for (Set<Constraint>::Element *E = removed_cells_constraints_set.front(); E; E = E->next()) {
			Map<Vector2i, TileSet::CellNeighbor> sources_of_constraint = E->get().get_overlapping_coords_and_peering_bits();
			for (Map<Vector2i, TileSet::CellNeighbor>::Element *E_source_tile_of_constraint = sources_of_constraint.front(); E_source_tile_of_constraint; E_source_tile_of_constraint = E_source_tile_of_constraint->next()) {
				if (potential_to_replace.has(E_source_tile_of_constraint->key())) {
					source_tiles_of_constraint[E->get()].insert(E_source_tile_of_constraint->key());
				}
			}
		}

		to_replace_modified = false;
		for (Set<TileMapEditorTerrainsPlugin::Constraint>::Element *E = added_tiles_constraints_set.front(); E; E = E->next()) {
			Constraint c = E->get();
			// Check if we have a conflict in constraints.
			if (removed_cells_constraints_set.has(c) && removed_cells_constraints_set.find(c)->get().get_terrain() != c.get_terrain()) {
				// If we do, we search for a neighbor to remove.
				if (source_tiles_of_constraint.has(c) && !source_tiles_of_constraint[c].is_empty()) {
					// Remove it.
					Vector2i to_add_to_remove = source_tiles_of_constraint[c].front()->get();
					potential_to_replace.erase(to_add_to_remove);
					to_replace.insert(to_add_to_remove);
					to_replace_modified = true;
					for (Map<Constraint, Set<Vector2i>>::Element *E_source_tiles_of_constraint = source_tiles_of_constraint.front(); E_source_tiles_of_constraint; E_source_tiles_of_constraint = E_source_tiles_of_constraint->next()) {
						E_source_tiles_of_constraint->get().erase(to_add_to_remove);
					}
					break;
				}
			}
		}
	}

	// Combine all constraints together.
	Set<TileMapEditorTerrainsPlugin::Constraint> constraints = removed_cells_constraints_set;
	for (Set<TileMapEditorTerrainsPlugin::Constraint>::Element *E = added_tiles_constraints_set.front(); E; E = E->next()) {
		constraints.insert(E->get());
	}

	// Run WFC to fill the holes with the constraints.
	Map<Vector2i, TerrainsTilePattern> wfc_output = _wave_function_collapse(to_replace, p_terrain_set, constraints);

	// Use the WFC run for the output.
	for (Map<Vector2i, TerrainsTilePattern>::Element *E = wfc_output.front(); E; E = E->next()) {
		output[E->key()] = _get_random_tile_from_pattern(p_terrain_set, E->get());
	}

	// Override the WFC results to make sure at least the painted tiles are actually painted.
	for (Map<Vector2i, TerrainsTilePattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		output[E_to_paint->key()] = _get_random_tile_from_pattern(p_terrain_set, E_to_paint->get());
	}

	return output;
}

void TileMapEditorTerrainsPlugin::_stop_dragging() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Vector2 mpos = xform.affine_inverse().xform(CanvasItemEditor::get_singleton()->get_viewport_control()->get_local_mouse_position());

	switch (drag_type) {
		case DRAG_TYPE_PICK: {
			Vector2i coords = tile_map->world_to_map(mpos);
			TileMapCell tile = tile_map->get_cell(tile_map_layer, coords);

			if (terrain_tiles.has(tile)) {
				Array terrains_tile_pattern = _build_terrains_tile_pattern(terrain_tiles[tile]);

				// Find the tree item for the right terrain set.
				bool need_tree_item_switch = true;
				TreeItem *tree_item = terrains_tree->get_selected();
				if (tree_item) {
					Dictionary metadata_dict = tree_item->get_metadata(0);
					if (metadata_dict.has("terrain_set") && metadata_dict.has("terrain_id")) {
						int terrain_set = metadata_dict["terrain_set"];
						int terrain_id = metadata_dict["terrain_id"];
						if (per_terrain_terrains_tile_patterns[terrain_set][terrain_id].has(terrains_tile_pattern)) {
							need_tree_item_switch = false;
						}
					}
				}

				if (need_tree_item_switch) {
					for (tree_item = terrains_tree->get_root()->get_first_child(); tree_item; tree_item = tree_item->get_next_visible()) {
						Dictionary metadata_dict = tree_item->get_metadata(0);
						if (metadata_dict.has("terrain_set") && metadata_dict.has("terrain_id")) {
							int terrain_set = metadata_dict["terrain_set"];
							int terrain_id = metadata_dict["terrain_id"];
							if (per_terrain_terrains_tile_patterns[terrain_set][terrain_id].has(terrains_tile_pattern)) {
								// Found
								tree_item->select(0);
								_update_tiles_list();
								break;
							}
						}
					}
				}

				// Find the list item for the given tile.
				if (tree_item) {
					for (int i = 0; i < terrains_tile_list->get_item_count(); i++) {
						Dictionary metadata_dict = terrains_tile_list->get_item_metadata(i);
						TerrainsTilePattern in_meta_terrains_tile_pattern = metadata_dict["terrains_tile_pattern"];
						bool equals = true;
						for (int j = 0; j < terrains_tile_pattern.size(); j++) {
							if (terrains_tile_pattern[j] != in_meta_terrains_tile_pattern[j]) {
								equals = false;
								break;
							}
						}
						if (equals) {
							terrains_tile_list->select(i);
							break;
						}
					}
				} else {
					ERR_PRINT("Terrain tile not found.");
				}
			}
			picker_button->set_pressed(false);
		} break;
		case DRAG_TYPE_PAINT: {
			undo_redo->create_action(TTR("Paint terrain"));
			for (Map<Vector2i, TileMapCell>::Element *E = drag_modified.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, E->key(), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
			}
			undo_redo->commit_action(false);
		} break;
		default:
			break;
	}
	drag_type = DRAG_TYPE_NONE;
}

bool TileMapEditorTerrainsPlugin::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {
	if (!is_visible_in_tree()) {
		// If the bottom editor is not visible, we ignore inputs.
		return false;
	}

	if (CanvasItemEditor::get_singleton()->get_current_tool() != CanvasItemEditor::TOOL_SELECT) {
		return false;
	}

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return false;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return false;
	}

	if (tile_map_layer < 0) {
		return false;
	}
	ERR_FAIL_COND_V(tile_map_layer >= tile_map->get_layers_count(), false);

	// Get the selected terrain.
	TerrainsTilePattern selected_terrains_tile_pattern;
	int selected_terrain_set = -1;

	TreeItem *selected_tree_item = terrains_tree->get_selected();
	if (selected_tree_item && selected_tree_item->get_metadata(0)) {
		Dictionary metadata_dict = selected_tree_item->get_metadata(0);
		// Selected terrain
		selected_terrain_set = metadata_dict["terrain_set"];

		// Selected tile
		if (erase_button->is_pressed()) {
			selected_terrains_tile_pattern.clear();
			for (uint32_t i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
				TileSet::CellNeighbor side = TileSet::CellNeighbor(i);
				if (tile_set->is_valid_peering_bit_terrain(selected_terrain_set, side)) {
					selected_terrains_tile_pattern.push_back(-1);
				}
			}
		} else if (terrains_tile_list->is_anything_selected()) {
			metadata_dict = terrains_tile_list->get_item_metadata(terrains_tile_list->get_selected_items()[0]);
			selected_terrains_tile_pattern = metadata_dict["terrains_tile_pattern"];
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mm->get_position());

		switch (drag_type) {
			case DRAG_TYPE_PAINT: {
				if (selected_terrain_set >= 0) {
					Vector<Vector2i> line = TileMapEditor::get_line(tile_map, tile_map->world_to_map(drag_last_mouse_pos), tile_map->world_to_map(mpos));
					Map<Vector2i, TerrainsTilePattern> to_draw;
					for (int i = 0; i < line.size(); i++) {
						to_draw[line[i]] = selected_terrains_tile_pattern;
					}
					Map<Vector2i, TileMapCell> modified = _draw_terrains(to_draw, selected_terrain_set);
					for (Map<Vector2i, TileMapCell>::Element *E = modified.front(); E; E = E->next()) {
						if (!drag_modified.has(E->key())) {
							drag_modified[E->key()] = tile_map->get_cell(tile_map_layer, E->key());
						}
						tile_map->set_cell(tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
					}
				}
			} break;
			default:
				break;
		}
		drag_last_mouse_pos = mpos;
		CanvasItemEditor::get_singleton()->update_viewport();

		return true;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mb->get_position());

		if (mb->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (mb->is_pressed()) {
				// Pressed
				if (picker_button->is_pressed()) {
					drag_type = DRAG_TYPE_PICK;
				} else {
					// Paint otherwise.
					if (selected_terrain_set >= 0 && !selected_terrains_tile_pattern.is_empty() && tool_buttons_group->get_pressed_button() == paint_tool_button) {
						drag_type = DRAG_TYPE_PAINT;
						drag_start_mouse_pos = mpos;

						drag_modified.clear();

						Map<Vector2i, TerrainsTilePattern> terrains_to_draw;
						terrains_to_draw[tile_map->world_to_map(mpos)] = selected_terrains_tile_pattern;

						Map<Vector2i, TileMapCell> to_draw = _draw_terrains(terrains_to_draw, selected_terrain_set);
						for (Map<Vector2i, TileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
							drag_modified[E->key()] = tile_map->get_cell(tile_map_layer, E->key());
							tile_map->set_cell(tile_map_layer, E->key(), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
						}
					}
				}
			} else {
				// Released
				_stop_dragging();
			}

			CanvasItemEditor::get_singleton()->update_viewport();

			return true;
		}
		drag_last_mouse_pos = mpos;
	}

	return false;
}

TileMapEditorTerrainsPlugin::TerrainsTilePattern TileMapEditorTerrainsPlugin::_build_terrains_tile_pattern(TileData *p_tile_data) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return TerrainsTilePattern();
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return TerrainsTilePattern();
	}

	TerrainsTilePattern output;
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (tile_set->is_valid_peering_bit_terrain(p_tile_data->get_terrain_set(), TileSet::CellNeighbor(i))) {
			output.push_back(p_tile_data->get_peering_bit_terrain(TileSet::CellNeighbor(i)));
		}
	}
	return output;
}

void TileMapEditorTerrainsPlugin::_update_terrains_cache() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Compute the tile sides.
	tile_sides.clear();
	TileSet::TileShape shape = tile_set->get_tile_shape();
	if (shape == TileSet::TILE_SHAPE_SQUARE) {
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_RIGHT_SIDE);
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE);
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_LEFT_SIDE);
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_SIDE);
	} else if (shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE);
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE);
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
		tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
	} else {
		if (tile_set->get_tile_offset_axis() == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_RIGHT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_LEFT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
		} else {
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_SIDE);
			tile_sides.push_back(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
		}
	}

	// Organizes tiles into structures.
	per_terrain_terrains_tile_patterns_tiles.resize(tile_set->get_terrain_sets_count());
	per_terrain_terrains_tile_patterns.resize(tile_set->get_terrain_sets_count());
	for (int i = 0; i < tile_set->get_terrain_sets_count(); i++) {
		per_terrain_terrains_tile_patterns_tiles[i].clear();
		per_terrain_terrains_tile_patterns[i].resize(tile_set->get_terrains_count(i));
		for (int j = 0; j < (int)per_terrain_terrains_tile_patterns[i].size(); j++) {
			per_terrain_terrains_tile_patterns[i][j].clear();
		}
	}

	for (int source_index = 0; source_index < tile_set->get_source_count(); source_index++) {
		int source_id = tile_set->get_source_id(source_index);
		Ref<TileSetSource> source = tile_set->get_source(source_id);

		Ref<TileSetAtlasSource> atlas_source = source;
		if (atlas_source.is_valid()) {
			for (int tile_index = 0; tile_index < source->get_tiles_count(); tile_index++) {
				Vector2i tile_id = source->get_tile_id(tile_index);
				for (int alternative_index = 0; alternative_index < source->get_alternative_tiles_count(tile_id); alternative_index++) {
					int alternative_id = source->get_alternative_tile_id(tile_id, alternative_index);

					TileData *tile_data = Object::cast_to<TileData>(atlas_source->get_tile_data(tile_id, alternative_id));
					int terrain_set = tile_data->get_terrain_set();
					if (terrain_set >= 0) {
						ERR_FAIL_INDEX(terrain_set, (int)per_terrain_terrains_tile_patterns.size());

						TileMapCell cell;
						cell.source_id = source_id;
						cell.set_atlas_coords(tile_id);
						cell.alternative_tile = alternative_id;

						TerrainsTilePattern terrains_tile_pattern = _build_terrains_tile_pattern(tile_data);

						// Terrain bits.
						for (int i = 0; i < terrains_tile_pattern.size(); i++) {
							int terrain = terrains_tile_pattern[i];
							if (terrain >= 0 && terrain < (int)per_terrain_terrains_tile_patterns[terrain_set].size()) {
								per_terrain_terrains_tile_patterns[terrain_set][terrain].insert(terrains_tile_pattern);
								terrain_tiles[cell] = tile_data;
								per_terrain_terrains_tile_patterns_tiles[terrain_set][terrains_tile_pattern].insert(cell);
							}
						}
					}
				}
			}
		}
	}

	// Add the empty cell in the possible patterns and cells.
	for (int i = 0; i < tile_set->get_terrain_sets_count(); i++) {
		TerrainsTilePattern empty_pattern;
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			if (tile_set->is_valid_peering_bit_terrain(i, TileSet::CellNeighbor(j))) {
				empty_pattern.push_back(-1);
			}
		}

		TileMapCell empty_cell;
		empty_cell.source_id = TileSet::INVALID_SOURCE;
		empty_cell.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
		empty_cell.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		per_terrain_terrains_tile_patterns_tiles[i][empty_pattern].insert(empty_cell);
	}
}

void TileMapEditorTerrainsPlugin::_update_terrains_tree() {
	terrains_tree->clear();
	terrains_tree->create_item();

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Fill in the terrain list.
	Vector<Vector<Ref<Texture2D>>> icons = tile_set->generate_terrains_icons(Size2(16, 16) * EDSCALE);
	for (int terrain_set_index = 0; terrain_set_index < tile_set->get_terrain_sets_count(); terrain_set_index++) {
		// Add an item for the terrain set.
		TreeItem *terrain_set_tree_item = terrains_tree->create_item();
		String matches;
		if (tile_set->get_terrain_set_mode(terrain_set_index) == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			terrain_set_tree_item->set_icon(0, get_theme_icon(SNAME("TerrainMatchCornersAndSides"), SNAME("EditorIcons")));
			matches = String(TTR("Matches Corners and Sides"));
		} else if (tile_set->get_terrain_set_mode(terrain_set_index) == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			terrain_set_tree_item->set_icon(0, get_theme_icon(SNAME("TerrainMatchCorners"), SNAME("EditorIcons")));
			matches = String(TTR("Matches Corners Only"));
		} else {
			terrain_set_tree_item->set_icon(0, get_theme_icon(SNAME("TerrainMatchSides"), SNAME("EditorIcons")));
			matches = String(TTR("Matches Sides Only"));
		}
		terrain_set_tree_item->set_text(0, vformat("Terrain Set %d (%s)", terrain_set_index, matches));
		terrain_set_tree_item->set_selectable(0, false);

		for (int terrain_index = 0; terrain_index < tile_set->get_terrains_count(terrain_set_index); terrain_index++) {
			// Add the item to the terrain list.
			TreeItem *terrain_tree_item = terrains_tree->create_item(terrain_set_tree_item);
			terrain_tree_item->set_text(0, tile_set->get_terrain_name(terrain_set_index, terrain_index));
			terrain_tree_item->set_icon_max_width(0, 32 * EDSCALE);
			terrain_tree_item->set_icon(0, icons[terrain_set_index][terrain_index]);

			Dictionary metadata_dict;
			metadata_dict["terrain_set"] = terrain_set_index;
			metadata_dict["terrain_id"] = terrain_index;
			terrain_tree_item->set_metadata(0, metadata_dict);
		}
	}
}

void TileMapEditorTerrainsPlugin::_update_tiles_list() {
	terrains_tile_list->clear();

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	TreeItem *selected_tree_item = terrains_tree->get_selected();
	if (selected_tree_item && selected_tree_item->get_metadata(0)) {
		Dictionary metadata_dict = selected_tree_item->get_metadata(0);
		int selected_terrain_set = metadata_dict["terrain_set"];
		int selected_terrain_id = metadata_dict["terrain_id"];
		ERR_FAIL_INDEX(selected_terrain_set, (int)per_terrain_terrains_tile_patterns.size());
		ERR_FAIL_INDEX(selected_terrain_id, (int)per_terrain_terrains_tile_patterns[selected_terrain_set].size());

		// Sort the items in a map by the number of corresponding terrains.
		Map<int, Set<TerrainsTilePattern>> sorted;
		for (Set<TerrainsTilePattern>::Element *E = per_terrain_terrains_tile_patterns[selected_terrain_set][selected_terrain_id].front(); E; E = E->next()) {
			// Count the number of matching sides/terrains.
			int count = 0;

			for (int i = 0; i < E->get().size(); i++) {
				if (int(E->get()[i]) == selected_terrain_id) {
					count++;
				}
			}
			sorted[count].insert(E->get());
		}

		for (Map<int, Set<TerrainsTilePattern>>::Element *E_set = sorted.back(); E_set; E_set = E_set->prev()) {
			for (Set<TerrainsTilePattern>::Element *E = E_set->get().front(); E; E = E->next()) {
				TerrainsTilePattern terrains_tile_pattern = E->get();

				// Get the icon.
				Ref<Texture2D> icon;
				Rect2 region;
				bool transpose = false;

				double max_probability = -1.0;
				for (Set<TileMapCell>::Element *E_tile_map_cell = per_terrain_terrains_tile_patterns_tiles[selected_terrain_set][terrains_tile_pattern].front(); E_tile_map_cell; E_tile_map_cell = E_tile_map_cell->next()) {
					Ref<TileSetSource> source = tile_set->get_source(E_tile_map_cell->get().source_id);

					Ref<TileSetAtlasSource> atlas_source = source;
					if (atlas_source.is_valid()) {
						TileData *tile_data = Object::cast_to<TileData>(atlas_source->get_tile_data(E_tile_map_cell->get().get_atlas_coords(), E_tile_map_cell->get().alternative_tile));
						if (tile_data->get_probability() > max_probability) {
							icon = atlas_source->get_texture();
							region = atlas_source->get_tile_texture_region(E_tile_map_cell->get().get_atlas_coords());
							if (tile_data->get_flip_h()) {
								region.position.x += region.size.x;
								region.size.x = -region.size.x;
							}
							if (tile_data->get_flip_v()) {
								region.position.y += region.size.y;
								region.size.y = -region.size.y;
							}
							transpose = tile_data->get_transpose();
							max_probability = tile_data->get_probability();
						}
					}
				}

				// Create the ItemList's item.
				int item_index = terrains_tile_list->add_item("");
				terrains_tile_list->set_item_icon(item_index, icon);
				terrains_tile_list->set_item_icon_region(item_index, region);
				terrains_tile_list->set_item_icon_transposed(item_index, transpose);
				Dictionary list_metadata_dict;
				list_metadata_dict["terrains_tile_pattern"] = terrains_tile_pattern;
				terrains_tile_list->set_item_metadata(item_index, list_metadata_dict);
			}
		}
		if (terrains_tile_list->get_item_count() > 0) {
			terrains_tile_list->select(0);
		}
	}
}

void TileMapEditorTerrainsPlugin::edit(ObjectID p_tile_map_id, int p_tile_map_layer) {
	_stop_dragging(); // Avoids staying in a wrong drag state.

	tile_map_id = p_tile_map_id;
	tile_map_layer = p_tile_map_layer;

	_update_terrains_cache();
	_update_terrains_tree();
	_update_tiles_list();
}

TileMapEditorTerrainsPlugin::TileMapEditorTerrainsPlugin() {
	set_name("Terrains");

	HSplitContainer *tilemap_tab_terrains = memnew(HSplitContainer);
	tilemap_tab_terrains->set_h_size_flags(SIZE_EXPAND_FILL);
	tilemap_tab_terrains->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(tilemap_tab_terrains);

	terrains_tree = memnew(Tree);
	terrains_tree->set_h_size_flags(SIZE_EXPAND_FILL);
	terrains_tree->set_stretch_ratio(0.25);
	terrains_tree->set_custom_minimum_size(Size2i(70, 0) * EDSCALE);
	terrains_tree->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	terrains_tree->set_hide_root(true);
	terrains_tree->connect("item_selected", callable_mp(this, &TileMapEditorTerrainsPlugin::_update_tiles_list));
	tilemap_tab_terrains->add_child(terrains_tree);

	terrains_tile_list = memnew(ItemList);
	terrains_tile_list->set_h_size_flags(SIZE_EXPAND_FILL);
	terrains_tile_list->set_max_columns(0);
	terrains_tile_list->set_same_column_width(true);
	terrains_tile_list->set_fixed_icon_size(Size2(30, 30) * EDSCALE);
	terrains_tile_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	tilemap_tab_terrains->add_child(terrains_tile_list);

	// --- Toolbar ---
	toolbar = memnew(HBoxContainer);

	HBoxContainer *tilemap_tiles_tools_buttons = memnew(HBoxContainer);

	tool_buttons_group.instantiate();

	paint_tool_button = memnew(Button);
	paint_tool_button->set_flat(true);
	paint_tool_button->set_toggle_mode(true);
	paint_tool_button->set_button_group(tool_buttons_group);
	paint_tool_button->set_pressed(true);
	paint_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/paint_tool", "Paint", KEY_D));
	paint_tool_button->connect("pressed", callable_mp(this, &TileMapEditorTerrainsPlugin::_update_toolbar));
	tilemap_tiles_tools_buttons->add_child(paint_tool_button);

	toolbar->add_child(tilemap_tiles_tools_buttons);

	// -- TileMap tool settings --
	tools_settings = memnew(HBoxContainer);
	toolbar->add_child(tools_settings);

	tools_settings_vsep = memnew(VSeparator);
	tools_settings->add_child(tools_settings_vsep);

	// Picker
	picker_button = memnew(Button);
	picker_button->set_flat(true);
	picker_button->set_toggle_mode(true);
	picker_button->set_shortcut(ED_SHORTCUT("tiles_editor/picker", "Picker", KEY_P));
	picker_button->connect("pressed", callable_mp(CanvasItemEditor::get_singleton(), &CanvasItemEditor::update_viewport));
	tools_settings->add_child(picker_button);

	// Erase button.
	erase_button = memnew(Button);
	erase_button->set_flat(true);
	erase_button->set_toggle_mode(true);
	erase_button->set_shortcut(ED_SHORTCUT("tiles_editor/eraser", "Eraser", KEY_E));
	erase_button->connect("pressed", callable_mp(CanvasItemEditor::get_singleton(), &CanvasItemEditor::update_viewport));
	tools_settings->add_child(erase_button);
}

TileMapEditorTerrainsPlugin::~TileMapEditorTerrainsPlugin() {
}

void TileMapEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED:
			missing_tile_texture = get_theme_icon(SNAME("StatusWarning"), SNAME("EditorIcons"));
			warning_pattern_texture = get_theme_icon(SNAME("WarningPattern"), SNAME("EditorIcons"));
			advanced_menu_button->set_icon(get_theme_icon(SNAME("Tools"), SNAME("EditorIcons")));
			toggle_grid_button->set_icon(get_theme_icon(SNAME("Grid"), SNAME("EditorIcons")));
			toggle_grid_button->set_pressed(EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid"));
			toogle_highlight_selected_layer_button->set_icon(get_theme_icon(SNAME("TileMapHighlightSelected"), SNAME("EditorIcons")));
			break;
		case NOTIFICATION_INTERNAL_PROCESS:
			if (is_visible_in_tree() && tileset_changed_needs_update) {
				_update_bottom_panel();
				_update_layers_selection();
				tile_map_editor_plugins[tabs->get_current_tab()]->tile_set_changed();
				CanvasItemEditor::get_singleton()->update_viewport();
				tileset_changed_needs_update = false;
			}
			break;
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED:
			toggle_grid_button->set_pressed(EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid"));
			break;
		case NOTIFICATION_VISIBILITY_CHANGED:
			TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
			if (tile_map) {
				if (is_visible_in_tree()) {
					tile_map->set_selected_layer(tile_map_layer);
				} else {
					tile_map->set_selected_layer(-1);
				}
			}
			break;
	}
}

void TileMapEditor::_on_grid_toggled(bool p_pressed) {
	EditorSettings::get_singleton()->set("editors/tiles_editor/display_grid", p_pressed);
}

void TileMapEditor::_layers_selection_button_draw() {
	if (!has_theme_icon(SNAME("arrow"), SNAME("OptionButton"))) {
		return;
	}

	RID ci = layers_selection_button->get_canvas_item();
	Ref<Texture2D> arrow = Control::get_theme_icon(SNAME("arrow"), SNAME("OptionButton"));

	Color clr = Color(1, 1, 1);
	if (get_theme_constant(SNAME("modulate_arrow"))) {
		switch (layers_selection_button->get_draw_mode()) {
			case BaseButton::DRAW_PRESSED:
				clr = get_theme_color(SNAME("font_pressed_color"));
				break;
			case BaseButton::DRAW_HOVER:
				clr = get_theme_color(SNAME("font_hover_color"));
				break;
			case BaseButton::DRAW_DISABLED:
				clr = get_theme_color(SNAME("font_disabled_color"));
				break;
			default:
				clr = get_theme_color(SNAME("font_color"));
		}
	}

	Size2 size = layers_selection_button->get_size();

	Point2 ofs;
	if (is_layout_rtl()) {
		ofs = Point2(get_theme_constant(SNAME("arrow_margin"), SNAME("OptionButton")), int(Math::abs((size.height - arrow->get_height()) / 2)));
	} else {
		ofs = Point2(size.width - arrow->get_width() - get_theme_constant(SNAME("arrow_margin"), SNAME("OptionButton")), int(Math::abs((size.height - arrow->get_height()) / 2)));
	}
	Rect2 dst_rect = Rect2(ofs, arrow->get_size());
	if (!layers_selection_button->is_pressed()) {
		dst_rect.size = -dst_rect.size;
	}
	arrow->draw_rect(ci, dst_rect, false, clr);
}

void TileMapEditor::_layers_selection_button_pressed() {
	if (!layers_selection_popup->is_visible()) {
		Size2 size = layers_selection_popup->get_contents_minimum_size();
		size.x = MAX(size.x, layers_selection_button->get_size().x);
		layers_selection_popup->set_position(layers_selection_button->get_screen_position() - Size2(0, size.y * get_global_transform().get_scale().y));
		layers_selection_popup->set_size(size);
		layers_selection_popup->popup();
	} else {
		layers_selection_popup->hide();
	}
}

void TileMapEditor::_layers_selection_id_pressed(int p_id) {
	tile_map_layer = p_id;
	_update_layers_selection();
}

void TileMapEditor::_advanced_menu_button_id_pressed(int p_id) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (p_id == 0) { // Replace Tile Proxies
		undo_redo->create_action(TTR("Replace Tiles with Proxies"));
		for (int layer_index = 0; layer_index < tile_map->get_layers_count(); layer_index++) {
			TypedArray<Vector2i> used_cells = tile_map->get_used_cells(layer_index);
			for (int i = 0; i < used_cells.size(); i++) {
				Vector2i cell_coords = used_cells[i];
				TileMapCell from = tile_map->get_cell(layer_index, cell_coords);
				Array to_array = tile_set->map_tile_proxy(from.source_id, from.get_atlas_coords(), from.alternative_tile);
				TileMapCell to;
				to.source_id = to_array[0];
				to.set_atlas_coords(to_array[1]);
				to.alternative_tile = to_array[2];
				if (from != to) {
					undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, cell_coords, to.source_id, to.get_atlas_coords(), to.alternative_tile);
					undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, cell_coords, from.source_id, from.get_atlas_coords(), from.alternative_tile);
				}
			}
		}
		undo_redo->commit_action();
	}
}

void TileMapEditor::_update_bottom_panel() {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}
	Ref<TileSet> tile_set = tile_map->get_tileset();

	// Update the visibility of controls.
	missing_tileset_label->set_visible(!tile_set.is_valid());
	if (!tile_set.is_valid()) {
		for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
			tile_map_editor_plugins[i]->hide();
		}
	} else {
		for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
			tile_map_editor_plugins[i]->set_visible(i == tabs->get_current_tab());
		}
	}
}

Vector<Vector2i> TileMapEditor::get_line(TileMap *p_tile_map, Vector2i p_from_cell, Vector2i p_to_cell) {
	ERR_FAIL_COND_V(!p_tile_map, Vector<Vector2i>());

	Ref<TileSet> tile_set = p_tile_map->get_tileset();
	ERR_FAIL_COND_V(!tile_set.is_valid(), Vector<Vector2i>());

	if (tile_set->get_tile_shape() == TileSet::TILE_SHAPE_SQUARE) {
		return Geometry2D::bresenham_line(p_from_cell, p_to_cell);
	} else {
		// Adapt the bresenham line algorithm to half-offset shapes.
		// See this blog post: http://zvold.blogspot.com/2010/01/bresenhams-line-drawing-algorithm-on_26.html
		Vector<Point2i> points;

		bool transposed = tile_set->get_tile_offset_axis() == TileSet::TILE_OFFSET_AXIS_VERTICAL;
		p_from_cell = TileMap::transform_coords_layout(p_from_cell, tile_set->get_tile_offset_axis(), tile_set->get_tile_layout(), TileSet::TILE_LAYOUT_STACKED);
		p_to_cell = TileMap::transform_coords_layout(p_to_cell, tile_set->get_tile_offset_axis(), tile_set->get_tile_layout(), TileSet::TILE_LAYOUT_STACKED);
		if (transposed) {
			SWAP(p_from_cell.x, p_from_cell.y);
			SWAP(p_to_cell.x, p_to_cell.y);
		}

		Vector2i delta = p_to_cell - p_from_cell;
		delta = Vector2i(2 * delta.x + ABS(p_to_cell.y % 2) - ABS(p_from_cell.y % 2), delta.y);
		Vector2i sign = delta.sign();

		Vector2i current = p_from_cell;
		points.push_back(TileMap::transform_coords_layout(transposed ? Vector2i(current.y, current.x) : current, tile_set->get_tile_offset_axis(), TileSet::TILE_LAYOUT_STACKED, tile_set->get_tile_layout()));

		int err = 0;
		if (ABS(delta.y) < ABS(delta.x)) {
			Vector2i err_step = 3 * delta.abs();
			while (current != p_to_cell) {
				err += err_step.y;
				if (err > ABS(delta.x)) {
					if (sign.x == 0) {
						current += Vector2(sign.y, 0);
					} else {
						current += Vector2(bool(current.y % 2) ^ (sign.x < 0) ? sign.x : 0, sign.y);
					}
					err -= err_step.x;
				} else {
					current += Vector2i(sign.x, 0);
					err += err_step.y;
				}
				points.push_back(TileMap::transform_coords_layout(transposed ? Vector2i(current.y, current.x) : current, tile_set->get_tile_offset_axis(), TileSet::TILE_LAYOUT_STACKED, tile_set->get_tile_layout()));
			}
		} else {
			Vector2i err_step = delta.abs();
			while (current != p_to_cell) {
				err += err_step.x;
				if (err > 0) {
					if (sign.x == 0) {
						current += Vector2(0, sign.y);
					} else {
						current += Vector2(bool(current.y % 2) ^ (sign.x < 0) ? sign.x : 0, sign.y);
					}
					err -= err_step.y;
				} else {
					if (sign.x == 0) {
						current += Vector2(0, sign.y);
					} else {
						current += Vector2(bool(current.y % 2) ^ (sign.x > 0) ? -sign.x : 0, sign.y);
					}
					err += err_step.y;
				}
				points.push_back(TileMap::transform_coords_layout(transposed ? Vector2i(current.y, current.x) : current, tile_set->get_tile_offset_axis(), TileSet::TILE_LAYOUT_STACKED, tile_set->get_tile_layout()));
			}
		}

		return points;
	}
}

void TileMapEditor::_tile_map_changed() {
	tileset_changed_needs_update = true;
}

void TileMapEditor::_tab_changed(int p_tab_id) {
	// Make the plugin edit the correct tilemap.
	tile_map_editor_plugins[tabs->get_current_tab()]->edit(tile_map_id, tile_map_layer);

	// Update toolbar.
	for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
		tile_map_editor_plugins[i]->get_toolbar()->set_visible(i == p_tab_id);
	}

	// Update visible panel.
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map || !tile_map->get_tileset().is_valid()) {
		for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
			tile_map_editor_plugins[i]->hide();
		}
	} else {
		for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
			tile_map_editor_plugins[i]->set_visible(i == tabs->get_current_tab());
		}
	}

	// Graphical update.
	tile_map_editor_plugins[tabs->get_current_tab()]->update();
	CanvasItemEditor::get_singleton()->update_viewport();
}

void TileMapEditor::_layers_select_next_or_previous(bool p_next) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	if (tile_map->get_layers_count() < 1) {
		return;
	}

	if (tile_map_layer < 0) {
		tile_map_layer = 0;
	}

	int inc = p_next ? 1 : -1;
	int origin_layer = tile_map_layer;
	tile_map_layer = Math::posmod((tile_map_layer + inc), tile_map->get_layers_count());
	while (tile_map_layer != origin_layer) {
		if (tile_map->is_layer_enabled(tile_map_layer)) {
			break;
		}
		tile_map_layer = Math::posmod((tile_map_layer + inc), tile_map->get_layers_count());
	}

	_update_layers_selection();
}

void TileMapEditor::_update_layers_selection() {
	layers_selection_popup->clear();

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	// Update the selected layer.
	if (is_visible_in_tree() && tile_map->get_layers_count() >= 1) {
		tile_map_layer = CLAMP(tile_map_layer, 0, tile_map->get_layers_count() - 1);

		// Search for an enabled layer if the current one is not.
		int origin_layer = tile_map_layer;
		while (tile_map_layer >= 0 && !tile_map->is_layer_enabled(tile_map_layer)) {
			tile_map_layer--;
		}
		if (tile_map_layer < 0) {
			tile_map_layer = origin_layer;
			while (tile_map_layer < tile_map->get_layers_count() && !tile_map->is_layer_enabled(tile_map_layer)) {
				tile_map_layer++;
			}
		}
		if (tile_map_layer >= tile_map->get_layers_count()) {
			tile_map_layer = -1;
		}
	} else {
		tile_map_layer = -1;
	}
	tile_map->set_selected_layer(toogle_highlight_selected_layer_button->is_pressed() ? tile_map_layer : -1);

	// Build the list of layers.
	for (int i = 0; i < tile_map->get_layers_count(); i++) {
		String name = tile_map->get_layer_name(i);
		layers_selection_popup->add_item(name.is_empty() ? vformat(TTR("Layer #%d"), i) : name, i);
		layers_selection_popup->set_item_as_radio_checkable(i, true);
		layers_selection_popup->set_item_disabled(i, !tile_map->is_layer_enabled(i));
		layers_selection_popup->set_item_checked(i, i == tile_map_layer);
	}

	// Update the button label.
	if (tile_map_layer >= 0) {
		layers_selection_button->set_text(layers_selection_popup->get_item_text(tile_map_layer));
	} else {
		layers_selection_button->set_text(TTR("Select a layer"));
	}

	// Set button minimum width.
	Size2 min_button_size = Size2(layers_selection_popup->get_contents_minimum_size().x, 0);
	if (has_theme_icon(SNAME("arrow"), SNAME("OptionButton"))) {
		Ref<Texture2D> arrow = Control::get_theme_icon(SNAME("arrow"), SNAME("OptionButton"));
		min_button_size.x += arrow->get_size().x;
	}
	layers_selection_button->set_custom_minimum_size(min_button_size);
	layers_selection_button->update();

	tile_map_editor_plugins[tabs->get_current_tab()]->edit(tile_map_id, tile_map_layer);
}

void TileMapEditor::_move_tile_map_array_element(Object *p_undo_redo, Object *p_edited, String p_array_prefix, int p_from_index, int p_to_pos) {
	UndoRedo *undo_redo = Object::cast_to<UndoRedo>(p_undo_redo);
	ERR_FAIL_COND(!undo_redo);

	TileMap *tile_map = Object::cast_to<TileMap>(p_edited);
	if (!tile_map) {
		return;
	}

	// Compute the array indices to save.
	int begin = 0;
	int end;
	if (p_array_prefix == "layer_") {
		end = tile_map->get_layers_count();
	} else {
		ERR_FAIL_MSG("Invalid array prefix for TileSet.");
	}
	if (p_from_index < 0) {
		// Adding new.
		if (p_to_pos >= 0) {
			begin = p_to_pos;
		} else {
			end = 0; // Nothing to save when adding at the end.
		}
	} else if (p_to_pos < 0) {
		// Removing.
		begin = p_from_index;
	} else {
		// Moving.
		begin = MIN(p_from_index, p_to_pos);
		end = MIN(MAX(p_from_index, p_to_pos) + 1, end);
	}

#define ADD_UNDO(obj, property) undo_redo->add_undo_property(obj, property, obj->get(property));
	// Save layers' properties.
	if (p_from_index < 0) {
		undo_redo->add_undo_method(tile_map, "remove_layer", p_to_pos < 0 ? tile_map->get_layers_count() : p_to_pos);
	} else if (p_to_pos < 0) {
		undo_redo->add_undo_method(tile_map, "add_layer", p_from_index);
	}

	List<PropertyInfo> properties;
	tile_map->get_property_list(&properties);
	for (PropertyInfo pi : properties) {
		if (pi.name.begins_with(p_array_prefix)) {
			String str = pi.name.trim_prefix(p_array_prefix);
			int to_char_index = 0;
			while (to_char_index < str.length()) {
				if (str[to_char_index] < '0' || str[to_char_index] > '9') {
					break;
				}
				to_char_index++;
			}
			if (to_char_index > 0) {
				int array_index = str.left(to_char_index).to_int();
				if (array_index >= begin && array_index < end) {
					ADD_UNDO(tile_map, pi.name);
				}
			}
		}
	}
#undef ADD_UNDO

	if (p_from_index < 0) {
		undo_redo->add_do_method(tile_map, "add_layer", p_to_pos);
	} else if (p_to_pos < 0) {
		undo_redo->add_do_method(tile_map, "remove_layer", p_from_index);
	} else {
		undo_redo->add_do_method(tile_map, "move_layer", p_from_index, p_to_pos);
	}
}

bool TileMapEditor::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {
	if (ED_IS_SHORTCUT("tiles_editor/select_next_layer", p_event) && p_event->is_pressed()) {
		_layers_select_next_or_previous(true);
		return true;
	}

	if (ED_IS_SHORTCUT("tiles_editor/select_previous_layer", p_event) && p_event->is_pressed()) {
		_layers_select_next_or_previous(false);
		return true;
	}

	return tile_map_editor_plugins[tabs->get_current_tab()]->forward_canvas_gui_input(p_event);
}

void TileMapEditor::forward_canvas_draw_over_viewport(Control *p_overlay) {
	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<TileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (!tile_map->is_visible_in_tree()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Transform2D xform_inv = xform.affine_inverse();
	Vector2i tile_shape_size = tile_set->get_tile_size();

	// Draw tiles with invalid IDs in the grid.
	if (tile_map_layer >= 0) {
		ERR_FAIL_COND(tile_map_layer >= tile_map->get_layers_count());
		TypedArray<Vector2i> used_cells = tile_map->get_used_cells(tile_map_layer);
		for (int i = 0; i < used_cells.size(); i++) {
			Vector2i coords = used_cells[i];
			int tile_source_id = tile_map->get_cell_source_id(tile_map_layer, coords);
			if (tile_source_id >= 0) {
				Vector2i tile_atlas_coords = tile_map->get_cell_atlas_coords(tile_map_layer, coords);
				int tile_alternative_tile = tile_map->get_cell_alternative_tile(tile_map_layer, coords);

				TileSetSource *source = nullptr;
				if (tile_set->has_source(tile_source_id)) {
					source = *tile_set->get_source(tile_source_id);
				}

				if (!source || !source->has_tile(tile_atlas_coords) || !source->has_alternative_tile(tile_atlas_coords, tile_alternative_tile)) {
					// Generate a random color from the hashed values of the tiles.
					Array a = tile_set->map_tile_proxy(tile_source_id, tile_atlas_coords, tile_alternative_tile);
					if (int(a[0]) == tile_source_id && Vector2i(a[1]) == tile_atlas_coords && int(a[2]) == tile_alternative_tile) {
						// Only display the pattern if we have no proxy tile.
						Array to_hash;
						to_hash.push_back(tile_source_id);
						to_hash.push_back(tile_atlas_coords);
						to_hash.push_back(tile_alternative_tile);
						uint32_t hash = RandomPCG(to_hash.hash()).rand();

						Color color;
						color = color.from_hsv(
								(float)((hash >> 24) & 0xFF) / 256.0,
								Math::lerp(0.5, 1.0, (float)((hash >> 16) & 0xFF) / 256.0),
								Math::lerp(0.5, 1.0, (float)((hash >> 8) & 0xFF) / 256.0),
								0.8);

						// Draw the scaled tile.
						Transform2D tile_xform;
						tile_xform.set_origin(tile_map->map_to_world(coords));
						tile_xform.set_scale(tile_shape_size);
						tile_set->draw_tile_shape(p_overlay, xform * tile_xform, color, true, warning_pattern_texture);
					}

					// Draw the warning icon.
					int min_axis = missing_tile_texture->get_size().min_axis();
					Vector2 icon_size;
					icon_size[min_axis] = tile_set->get_tile_size()[min_axis] / 3;
					icon_size[(min_axis + 1) % 2] = (icon_size[min_axis] * missing_tile_texture->get_size()[(min_axis + 1) % 2] / missing_tile_texture->get_size()[min_axis]);
					Rect2 rect = Rect2(xform.xform(tile_map->map_to_world(coords)) - (icon_size * xform.get_scale() / 2), icon_size * xform.get_scale());
					p_overlay->draw_texture_rect(missing_tile_texture, rect);
				}
			}
		}
	}

	// Fading on the border.
	const int fading = 5;

	// Determine the drawn area.
	Size2 screen_size = p_overlay->get_size();
	Rect2i screen_rect;
	screen_rect.position = tile_map->world_to_map(xform_inv.xform(Vector2()));
	screen_rect.expand_to(tile_map->world_to_map(xform_inv.xform(Vector2(0, screen_size.height))));
	screen_rect.expand_to(tile_map->world_to_map(xform_inv.xform(Vector2(screen_size.width, 0))));
	screen_rect.expand_to(tile_map->world_to_map(xform_inv.xform(screen_size)));
	screen_rect = screen_rect.grow(1);

	Rect2i tilemap_used_rect = tile_map->get_used_rect();

	Rect2i displayed_rect = tilemap_used_rect.intersection(screen_rect);
	displayed_rect = displayed_rect.grow(fading);

	// Reduce the drawn area to avoid crashes if needed.
	int max_size = 100;
	if (displayed_rect.size.x > max_size) {
		displayed_rect = displayed_rect.grow_individual(-(displayed_rect.size.x - max_size) / 2, 0, -(displayed_rect.size.x - max_size) / 2, 0);
	}
	if (displayed_rect.size.y > max_size) {
		displayed_rect = displayed_rect.grow_individual(0, -(displayed_rect.size.y - max_size) / 2, 0, -(displayed_rect.size.y - max_size) / 2);
	}

	// Draw the grid.
	bool display_grid = EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid");
	if (display_grid) {
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		for (int x = displayed_rect.position.x; x < (displayed_rect.position.x + displayed_rect.size.x); x++) {
			for (int y = displayed_rect.position.y; y < (displayed_rect.position.y + displayed_rect.size.y); y++) {
				Vector2i pos_in_rect = Vector2i(x, y) - displayed_rect.position;

				// Fade out the border of the grid.
				float left_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.x), 0.0f, 1.0f);
				float right_opacity = CLAMP(Math::inverse_lerp((float)displayed_rect.size.x, (float)(displayed_rect.size.x - fading), (float)pos_in_rect.x), 0.0f, 1.0f);
				float top_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.y), 0.0f, 1.0f);
				float bottom_opacity = CLAMP(Math::inverse_lerp((float)displayed_rect.size.y, (float)(displayed_rect.size.y - fading), (float)pos_in_rect.y), 0.0f, 1.0f);
				float opacity = CLAMP(MIN(left_opacity, MIN(right_opacity, MIN(top_opacity, bottom_opacity))) + 0.1, 0.0f, 1.0f);

				Transform2D tile_xform;
				tile_xform.set_origin(tile_map->map_to_world(Vector2(x, y)));
				tile_xform.set_scale(tile_shape_size);
				Color color = grid_color;
				color.a = color.a * opacity;
				tile_set->draw_tile_shape(p_overlay, xform * tile_xform, color, false);
			}
		}
	}

	// Draw the IDs for debug.
	/*Ref<Font> font = get_theme_font(SNAME("font"), SNAME("Label"));
	for (int x = displayed_rect.position.x; x < (displayed_rect.position.x + displayed_rect.size.x); x++) {
		for (int y = displayed_rect.position.y; y < (displayed_rect.position.y + displayed_rect.size.y); y++) {
			p_overlay->draw_string(font, xform.xform(tile_map->map_to_world(Vector2(x, y))) + Vector2i(-tile_shape_size.x / 2, 0), vformat("%s", Vector2(x, y)));
		}
	}*/

	// Draw the plugins.
	tile_map_editor_plugins[tabs->get_current_tab()]->forward_canvas_draw_over_viewport(p_overlay);
}

void TileMapEditor::edit(TileMap *p_tile_map) {
	if (p_tile_map && p_tile_map->get_instance_id() == tile_map_id) {
		return;
	}

	TileMap *tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
	if (tile_map) {
		// Unselect layer if we are changing tile_map.
		if (tile_map != p_tile_map) {
			tile_map->set_selected_layer(-1);
		}

		// Disconnect to changes.
		tile_map->disconnect("changed", callable_mp(this, &TileMapEditor::_tile_map_changed));
	}

	if (p_tile_map) {
		// Change the edited object.
		tile_map_id = p_tile_map->get_instance_id();
		tile_map = Object::cast_to<TileMap>(ObjectDB::get_instance(tile_map_id));
		// Connect to changes.
		if (!tile_map->is_connected("changed", callable_mp(this, &TileMapEditor::_tile_map_changed))) {
			tile_map->connect("changed", callable_mp(this, &TileMapEditor::_tile_map_changed));
		}
	} else {
		tile_map_id = ObjectID();
	}

	_update_layers_selection();

	// Call the plugins.
	tile_map_editor_plugins[tabs->get_current_tab()]->edit(tile_map_id, tile_map_layer);

	_tile_map_changed();
}

TileMapEditor::TileMapEditor() {
	set_process_internal(true);

	// Shortcuts.
	ED_SHORTCUT("tiles_editor/select_next_layer", TTR("Select Next Tile Map Layer"), KEY_PAGEUP);
	ED_SHORTCUT("tiles_editor/select_previous_layer", TTR("Select Previous Tile Map Layer"), KEY_PAGEDOWN);

	// TileMap editor plugins
	tile_map_editor_plugins.push_back(memnew(TileMapEditorTilesPlugin));
	tile_map_editor_plugins.push_back(memnew(TileMapEditorTerrainsPlugin));

	// Tabs.
	tabs = memnew(Tabs);
	tabs->set_clip_tabs(false);
	for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
		tabs->add_tab(tile_map_editor_plugins[i]->get_name());
	}
	tabs->connect("tab_changed", callable_mp(this, &TileMapEditor::_tab_changed));

	// --- TileMap toolbar ---
	tile_map_toolbar = memnew(HBoxContainer);
	tile_map_toolbar->set_h_size_flags(SIZE_EXPAND_FILL);

	// Tabs.
	tile_map_toolbar->add_child(tabs);

	// Tabs toolbars.
	for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
		tile_map_editor_plugins[i]->get_toolbar()->hide();
		tile_map_toolbar->add_child(tile_map_editor_plugins[i]->get_toolbar());
	}

	// Wide empty separation control.
	Control *h_empty_space = memnew(Control);
	h_empty_space->set_h_size_flags(SIZE_EXPAND_FILL);
	tile_map_toolbar->add_child(h_empty_space);

	// Layer selector.
	layers_selection_popup = memnew(PopupMenu);
	layers_selection_popup->connect("id_pressed", callable_mp(this, &TileMapEditor::_layers_selection_id_pressed));
	layers_selection_popup->set_close_on_parent_focus(false);

	layers_selection_button = memnew(Button);
	layers_selection_button->set_toggle_mode(true);
	layers_selection_button->connect("draw", callable_mp(this, &TileMapEditor::_layers_selection_button_draw));
	layers_selection_button->connect("pressed", callable_mp(this, &TileMapEditor::_layers_selection_button_pressed));
	layers_selection_button->connect("hidden", callable_mp((Window *)layers_selection_popup, &Popup::hide));
	layers_selection_button->set_tooltip(TTR("Tile Map Layer"));
	layers_selection_button->add_child(layers_selection_popup);
	tile_map_toolbar->add_child(layers_selection_button);

	toogle_highlight_selected_layer_button = memnew(Button);
	toogle_highlight_selected_layer_button->set_flat(true);
	toogle_highlight_selected_layer_button->set_toggle_mode(true);
	toogle_highlight_selected_layer_button->set_pressed(true);
	toogle_highlight_selected_layer_button->connect("pressed", callable_mp(this, &TileMapEditor::_update_layers_selection));
	toogle_highlight_selected_layer_button->set_tooltip(TTR("Highlight Selected TileMap Layer"));
	tile_map_toolbar->add_child(toogle_highlight_selected_layer_button);

	tile_map_toolbar->add_child(memnew(VSeparator));

	// Grid toggle.
	toggle_grid_button = memnew(Button);
	toggle_grid_button->set_flat(true);
	toggle_grid_button->set_toggle_mode(true);
	toggle_grid_button->set_tooltip(TTR("Toggle grid visibility."));
	toggle_grid_button->connect("toggled", callable_mp(this, &TileMapEditor::_on_grid_toggled));
	tile_map_toolbar->add_child(toggle_grid_button);

	// Advanced settings menu button.
	advanced_menu_button = memnew(MenuButton);
	advanced_menu_button->set_flat(true);
	advanced_menu_button->get_popup()->add_item(TTR("Automatically Replace Tiles with Proxies"));
	advanced_menu_button->get_popup()->connect("id_pressed", callable_mp(this, &TileMapEditor::_advanced_menu_button_id_pressed));
	tile_map_toolbar->add_child(advanced_menu_button);

	missing_tileset_label = memnew(Label);
	missing_tileset_label->set_text(TTR("The edited TileMap node has no TileSet resource."));
	missing_tileset_label->set_h_size_flags(SIZE_EXPAND_FILL);
	missing_tileset_label->set_v_size_flags(SIZE_EXPAND_FILL);
	missing_tileset_label->set_align(Label::ALIGN_CENTER);
	missing_tileset_label->set_valign(Label::VALIGN_CENTER);
	missing_tileset_label->hide();
	add_child(missing_tileset_label);

	for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
		add_child(tile_map_editor_plugins[i]);
		tile_map_editor_plugins[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		tile_map_editor_plugins[i]->set_v_size_flags(SIZE_EXPAND_FILL);
		tile_map_editor_plugins[i]->set_visible(i == 0);
	}

	_tab_changed(0);

	// Registers UndoRedo inspector callback.
	EditorNode::get_singleton()->get_editor_data().add_move_array_element_function(SNAME("TileMap"), callable_mp(this, &TileMapEditor::_move_tile_map_array_element));
}

TileMapEditor::~TileMapEditor() {
}
