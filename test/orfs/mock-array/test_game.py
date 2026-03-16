#!/usr/bin/env python3
"""Unit tests for DRC Destroyer game.py — with mocked odb module."""

import json
import os
import tempfile
import unittest
from unittest.mock import MagicMock, patch

import game


class TestParseLogWarnings(unittest.TestCase):
    def test_extracts_known_warnings(self):
        with tempfile.TemporaryDirectory() as d:
            log = os.path.join(d, "2_1_floorplan.log")
            with open(log, "w") as f:
                f.write(
                    "Warning: There are 1025 input ports missing set_input_delay.\n"
                    "Warning: There are 732 unconstrained endpoints.\n"
                    "[WARNING EST-0027] no estimated parasitics.\n"
                )
            villains = game.parse_log_warnings(d)
            names = [v["name"] for v in villains]
            self.assertIn("Lord Set_Input_Delay", names)
            self.assertIn("The Unconstrained 732", names)
            self.assertIn("Commander No_Parasitics", names)

    def test_empty_dir_returns_empty(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(game.parse_log_warnings(d), [])

    def test_none_dir_returns_empty(self):
        self.assertEqual(game.parse_log_warnings(None), [])

    def test_deduplicates_warnings(self):
        with tempfile.TemporaryDirectory() as d:
            for name in ("a.log", "b.log"):
                with open(os.path.join(d, name), "w") as f:
                    f.write("[WARNING EST-0027] no estimated parasitics.\n")
            villains = game.parse_log_warnings(d)
            names = [v["name"] for v in villains]
            self.assertEqual(names.count("Commander No_Parasitics"), 1)


class TestParseDrcLog(unittest.TestCase):
    def test_parses_violation_table(self):
        with tempfile.TemporaryDirectory() as d:
            log = os.path.join(d, "5_2_route.log")
            with open(log, "w") as f:
                f.write(
                    "[INFO DRT-0199]   Number of violations = 34.\n"
                    "Viol/Layer          M2     M3     V3     V4\n"
                    "CutSpcTbl            0      0      4     24\n"
                    "Metal Spacing        0      4      0      0\n"
                    "Short                1      1      0      0\n"
                    "[INFO DRT-0267] cpu time\n"
                )
            villains = game.parse_drc_log(d)
            names = [v["name"] for v in villains]
            self.assertIn("Cut Spacing the Merciless", names)
            self.assertIn("Metal Spacing the Crusher", names)
            self.assertIn("Short Circuit the Terrible", names)
            cut = next(v for v in villains if "Cut Spacing" in v["name"])
            self.assertEqual(cut["count"], 28)

    def test_no_route_log_returns_empty(self):
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "other.log"), "w") as f:
                f.write("nothing\n")
            self.assertEqual(game.parse_drc_log(d), [])


class TestParseTiming(unittest.TestCase):
    def test_reads_metadata_json(self):
        with tempfile.TemporaryDirectory() as d:
            meta = {
                "finish__timing__setup__ws": 11.1,
                "finish__timing__hold__ws": 27.2,
                "finish__timing__fmax": 11.28e9,
            }
            with open(os.path.join(d, "metadata.json"), "w") as f:
                json.dump(meta, f)
            timing = game.parse_timing(d)
            self.assertAlmostEqual(timing["setup_ws"], 11.1)
            self.assertAlmostEqual(timing["fmax"], 11.28e9)

    def test_missing_metadata_returns_nones(self):
        with tempfile.TemporaryDirectory() as d:
            timing = game.parse_timing(d)
            self.assertIsNone(timing["setup_ws"])


class TestGenerateDemoData(unittest.TestCase):
    def test_produces_valid_structure(self):
        data = game.generate_demo_data()
        self.assertIn("die", data)
        self.assertIn("drc", data)
        self.assertIn("wires", data)
        self.assertIn("cells", data)
        self.assertIn("design_name", data)
        self.assertGreater(len(data["drc"]), 0)
        self.assertGreater(len(data["wires"]), 0)

    def test_drc_markers_have_required_fields(self):
        data = game.generate_demo_data()
        for d in data["drc"]:
            self.assertIn("type", d)
            self.assertIn("xMin", d)
            self.assertIn("layer", d)


class TestBuildIntroText(unittest.TestCase):
    def test_includes_design_name(self):
        lines = game.build_intro_text("TestChip", [{"type": "Short"}], [], [], {})
        text = "\n".join(lines)
        self.assertIn("TestChip", text)

    def test_includes_drc_count(self):
        markers = [{"type": "Short"}] * 42
        lines = game.build_intro_text("X", markers, [], [], {})
        text = "\n".join(lines)
        self.assertIn("42", text)

    def test_training_exercise_when_no_drc(self):
        lines = game.build_intro_text("X", [], [], [], {})
        text = "\n".join(lines)
        self.assertIn("TRAINING EXERCISE", text)
        self.assertIn("virtuous saviours", text)

    def test_negative_slack_villain(self):
        timing = {"setup_ws": -5.3, "hold_ws": 10.0, "fmax": 1e9}
        lines = game.build_intro_text("X", [{"type": "S"}], [], [], timing)
        text = "\n".join(lines)
        self.assertIn("NEGATIVE SLACK", text)
        self.assertIn("-5.3", text)

    def test_positive_slack_ally(self):
        timing = {"setup_ws": 11.1, "hold_ws": 27.2, "fmax": 11.28e9}
        lines = game.build_intro_text("X", [{"type": "S"}], [], [], timing)
        text = "\n".join(lines)
        self.assertIn("Slack is with you", text)

    def test_includes_democratization_message(self):
        lines = game.build_intro_text("X", [], [], [], {})
        text = "\n".join(lines)
        self.assertIn("democratized", text)
        self.assertIn("zero programming", text)


class TestVillainGeneration(unittest.TestCase):
    def test_known_drc_types_get_named(self):
        self.assertEqual(game.DRC_VILLAIN_NAMES["Short"], "Short Circuit the Terrible")
        self.assertEqual(game.DRC_VILLAIN_NAMES["EOL"], "EOL the Edgelord")

    def test_unknown_type_gets_fallback(self):
        with tempfile.TemporaryDirectory() as d:
            log = os.path.join(d, "5_2_route.log")
            with open(log, "w") as f:
                f.write(
                    "[INFO DRT-0199]   Number of violations = 5.\n"
                    "Viol/Layer          M2\n"
                    "WeirdNewRule         5\n"
                    "[INFO DRT-0267] done\n"
                )
            villains = game.parse_drc_log(d)
            self.assertEqual(len(villains), 1)
            self.assertIn("Villainous", villains[0]["name"])


class TestHtmlOutput(unittest.TestCase):
    def test_generates_valid_html(self):
        data = game.generate_demo_data()
        intro = ["DRC DESTROYER", "test"]
        warnings = [{"name": "Test", "quote": "hi"}]
        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            game.generate_html(data, intro, warnings, path)
            with open(path) as f:
                html = f.read()
            self.assertIn("<!DOCTYPE html>", html)
            self.assertIn("DRC Destroyer", html)
            self.assertIn("INSERT COIN", html)
            self.assertIn("canvas", html)
            self.assertIn("GAME_DATA", html)
            # Verify JSON is valid
            json_match = html.split("const GAME_DATA = ")[1].split(";\n")[0]
            parsed = json.loads(json_match)
            self.assertIn("die", parsed)
        finally:
            os.unlink(path)

    def test_contains_autopilot_logic(self):
        data = game.generate_demo_data()
        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            game.generate_html(data, [], [], path)
            with open(path) as f:
                html = f.read()
            self.assertIn("PILOT ASLEEP", html)
            self.assertIn("AUTOPILOT", html)
            self.assertIn("BONUS", html)
            self.assertIn("LEVEL COMPLETE", html)
        finally:
            os.unlink(path)


class TestWireSampling(unittest.TestCase):
    def test_budget_respected(self):
        # Generate data with many wires then check budget
        data = game.generate_demo_data()
        self.assertLessEqual(len(data["wires"]), game.WIRE_BUDGET)


class TestExtractFromOdb(unittest.TestCase):
    @patch.dict("sys.modules", {"odb": MagicMock()})
    def test_extracts_markers_and_wires(self):
        import sys

        mock_odb_module = sys.modules["odb"]

        # Set up mock ODB
        mock_db = MagicMock()
        mock_odb_module.dbDatabase.create.return_value = mock_db

        mock_block = MagicMock()
        mock_block.getName.return_value = "TestDesign"

        mock_die = MagicMock()
        mock_die.xMin.return_value = 0
        mock_die.yMin.return_value = 0
        mock_die.xMax.return_value = 100000
        mock_die.yMax.return_value = 100000
        mock_block.getDieArea.return_value = mock_die

        # Mock markers
        mock_marker = MagicMock()
        mock_bbox = MagicMock()
        mock_bbox.xMin.return_value = 1000
        mock_bbox.yMin.return_value = 2000
        mock_bbox.xMax.return_value = 1500
        mock_bbox.yMax.return_value = 2500
        mock_marker.getBBox.return_value = mock_bbox
        mock_layer = MagicMock()
        mock_layer.getName.return_value = "M3"
        mock_marker.getTechLayer.return_value = mock_layer

        mock_subcat = MagicMock()
        mock_subcat.getName.return_value = "Short"
        mock_subcat.getMarkers.return_value = [mock_marker]
        mock_subcat.getMarkerCategories.return_value = []

        mock_cat = MagicMock()
        mock_cat.getMarkers.return_value = []
        mock_cat.getMarkerCategories.return_value = [mock_subcat]
        mock_block.getMarkerCategories.return_value = [mock_cat]

        # Mock no wires (empty nets)
        mock_block.getNets.return_value = []

        # Mock instances
        mock_inst = MagicMock()
        mock_inst.getMaster.return_value.getName.return_value = "BUFx2_ASAP7_75t_R"
        mock_block.getInsts.return_value = [mock_inst]

        mock_chip = MagicMock()
        mock_chip.getBlock.return_value = mock_block
        mock_db.getChip.return_value = mock_chip

        result = game.extract_from_odb("fake.odb")

        self.assertIsNotNone(result)
        self.assertEqual(result["design_name"], "TestDesign")
        self.assertEqual(len(result["drc"]), 1)
        self.assertEqual(result["drc"][0]["type"], "Short")
        self.assertEqual(result["drc"][0]["layer"], "M3")
        self.assertIn("BUFx2_ASAP7_75t_R", result["cells"])


# ---------------------------------------------------------------------------
# Game Logic Tests (verifying the JavaScript game behavior via HTML structure)
# ---------------------------------------------------------------------------


class TestGameJsLogic(unittest.TestCase):
    """Test the generated JS game logic by inspecting the HTML output."""

    def setUp(self):
        self.data = game.generate_demo_data()
        self.tmpfile = tempfile.NamedTemporaryFile(suffix=".html", delete=False)
        self.tmpfile.close()
        game.generate_html(self.data, ["DRC DESTROYER"], [], self.tmpfile.name)
        with open(self.tmpfile.name) as f:
            self.html = f.read()

    def tearDown(self):
        os.unlink(self.tmpfile.name)

    def test_game_states_defined(self):
        """Game must have intro, attract, and play states."""
        self.assertIn('state = "intro"', self.html)
        self.assertIn('"attract"', self.html)
        self.assertIn('"play"', self.html)

    def test_intro_skippable(self):
        """Pressing any key during intro transitions to attract."""
        self.assertIn('state === "intro"', self.html)
        self.assertIn('state = "attract"', self.html)

    def test_attract_to_play_on_arrow(self):
        """Arrow keys during attract start play mode."""
        self.assertIn('state === "attract"', self.html)
        self.assertIn('state = "play"', self.html)
        self.assertIn("ArrowLeft", self.html)

    def test_manual_mode_toggle(self):
        """D key toggles manual/autopilot."""
        self.assertIn('"d"', self.html)
        self.assertIn("manualMode = !manualMode", self.html)

    def test_bomb_drop_on_space(self):
        """Space bar drops a bomb in manual mode."""
        self.assertIn('" "', self.html)
        self.assertIn("bombs.push", self.html)

    def test_bomber_movement_clamped(self):
        """Bomber position clamped to 0-1 range."""
        self.assertIn("Math.max(0, Math.min(1, camNx))", self.html)
        self.assertIn("Math.max(0, Math.min(1, camNy))", self.html)

    def test_detonation_destroys_targets(self):
        """Bombs detonate and destroy nearby DRC targets."""
        self.assertIn("function detonate", self.html)
        self.assertIn("t.alive = false", self.html)

    def test_score_bonus_for_manual(self):
        """Manual kills score x2, autopilot x1."""
        self.assertIn("bonusActive ? 20 : 10", self.html)

    def test_pilot_asleep_after_inactivity(self):
        """Pilot falls asleep after 150 ticks (~5s) of no input."""
        self.assertIn("tick - lastManualInput > 150", self.html)
        self.assertIn("pilotAsleep = true", self.html)
        self.assertIn("bonusActive = false", self.html)

    def test_pilot_wakes_on_input(self):
        """Arrow key wakes the pilot."""
        self.assertIn("pilotAsleep = false", self.html)

    def test_autopilot_seeks_targets(self):
        """Autopilot navigates toward alive targets."""
        self.assertIn("aliveTargets", self.html)
        self.assertIn("autopilotTarget", self.html)

    def test_level_complete_when_all_cleared(self):
        """Level complete triggers when DRC + warnings all destroyed."""
        self.assertIn("LEVEL COMPLETE", self.html)
        self.assertIn("drcLeft === 0 && warnLeft === 0", self.html)

    def test_insert_coin_flashes(self):
        """INSERT COIN text flashes during attract mode."""
        self.assertIn("INSERT COIN", self.html)
        self.assertIn("tick / 45", self.html)

    def test_controls_legend_exists(self):
        """Controls legend drawn with black background in upper right."""
        self.assertIn("drawControlsLegend", self.html)
        self.assertIn("CONTROLS", self.html)
        self.assertIn("Start / Bomb", self.html)
        self.assertIn("Autopilot", self.html)

    def test_training_targets_when_no_drc(self):
        """If no DRC markers, training targets are generated in JS."""
        self.assertIn("targets.length === 0", self.html)
        self.assertIn('"Training"', self.html)

    def test_explosion_lifecycle(self):
        """Explosions spawn on bomb impact and expire after duration."""
        self.assertIn("explosions.push", self.html)
        self.assertIn("tick - explosions[i].tick > explosions[i].dur", self.html)

    def test_cell_ticker_scrolls(self):
        """Cell observation ticker generates messages and scrolls."""
        self.assertIn("tickerMessages", self.html)
        self.assertIn("msg.x -= 1.0", self.html)

    def test_warning_markers_destructible(self):
        """Warning markers can be destroyed by bombs."""
        self.assertIn("warningMarkers", self.html)
        self.assertIn("w.alive = false", self.html)

    def test_isometric_projection(self):
        """Isometric projection function exists."""
        self.assertIn("function iso(nx, ny, z)", self.html)

    def test_hud_displays_drc_countdown(self):
        """HUD shows remaining DRC and warning counts."""
        self.assertIn("DRC: ${drcLeft}", self.html)
        self.assertIn("WARNINGS: ${warnLeft}", self.html)

    def test_game_loop_uses_raf(self):
        """Game loop uses requestAnimationFrame for smooth rendering."""
        self.assertIn("requestAnimationFrame(loop)", self.html)


class TestGameDataIntegrity(unittest.TestCase):
    """Test that game data flows correctly into the HTML."""

    def test_drc_data_embedded_as_json(self):
        data = game.generate_demo_data()
        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            game.generate_html(data, [], [], path)
            with open(path) as f:
                html = f.read()
            # Extract and parse the embedded JSON
            json_str = html.split("const GAME_DATA = ")[1].split(";\n")[0]
            parsed = json.loads(json_str)
            self.assertEqual(len(parsed["drc"]), len(data["drc"]))
            self.assertEqual(parsed["design_name"], data["design_name"])
            self.assertEqual(parsed["die"], data["die"])
        finally:
            os.unlink(path)

    def test_intro_lines_embedded(self):
        data = game.generate_demo_data()
        intro = ["LINE ONE", "LINE TWO", "DRC DESTROYER"]
        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            game.generate_html(data, intro, [], path)
            with open(path) as f:
                html = f.read()
            json_str = html.split("const INTRO_LINES = ")[1].split(";\n")[0]
            parsed = json.loads(json_str)
            self.assertEqual(parsed, intro)
        finally:
            os.unlink(path)

    def test_warning_villains_embedded(self):
        data = game.generate_demo_data()
        warnings = [{"name": "Lord Test", "quote": "Testing!"}]
        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            game.generate_html(data, [], warnings, path)
            with open(path) as f:
                html = f.read()
            json_str = html.split("const WARNING_VILLAINS = ")[1].split(";\n")[0]
            parsed = json.loads(json_str)
            self.assertEqual(parsed[0]["name"], "Lord Test")
        finally:
            os.unlink(path)

    def test_layer_colors_embedded(self):
        data = game.generate_demo_data()
        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            game.generate_html(data, [], [], path)
            with open(path) as f:
                html = f.read()
            json_str = html.split("const LAYER_COLORS = ")[1].split(";\n")[0]
            parsed = json.loads(json_str)
            self.assertIn("M1", parsed)
            self.assertEqual(parsed["M1"], game.LAYER_COLORS["M1"])
        finally:
            os.unlink(path)


class TestEndToEndDemo(unittest.TestCase):
    """Test full pipeline: demo data -> intro -> HTML."""

    def test_full_demo_pipeline(self):
        data = game.generate_demo_data()
        warning_villains = [
            {"name": "Lord Set_Input_Delay", "quote": "1025 ports cry out!"},
        ]
        drc_villains = [
            {
                "name": "Short Circuit the Terrible",
                "type": "Short",
                "count": 165,
                "layers": ["M2", "M3"],
            },
        ]
        timing = {"setup_ws": 11.1, "hold_ws": 27.2, "fmax": 11.28e9}
        intro = game.build_intro_text(
            data["design_name"],
            data["drc"],
            warning_villains,
            drc_villains,
            timing,
        )

        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as f:
            path = f.name
        try:
            game.generate_html(data, intro, warning_villains, path)
            with open(path) as f:
                html = f.read()
            # Verify key elements present
            self.assertIn("DRC DESTROYER", html)
            self.assertIn("INSERT COIN", html)
            self.assertIn("LEVEL COMPLETE", html)
            self.assertIn("PILOT ASLEEP", html)
            self.assertIn("requestAnimationFrame", html)
            self.assertGreater(len(html), 5000)
        finally:
            os.unlink(path)


if __name__ == "__main__":
    unittest.main()
