import importlib.machinery
import importlib.util
import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]


def load(name, path):
    loader = importlib.machinery.SourceFileLoader(name, str(path))
    spec = importlib.util.spec_from_loader(name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


setup = load("fairwindsk_halos_setup", ROOT / "extras/fairwindsk-halos-setup")
startup = load("fairwindsk_startup", ROOT / "extras/fairwindsk-startup")


class HaLOSSetupTests(unittest.TestCase):
    def test_configuration_path_follows_existing_settings(self):
        with tempfile.TemporaryDirectory() as directory, mock.patch.dict(os.environ, {"XDG_CONFIG_HOME": directory}, clear=False):
            settings = Path(directory) / "FairWindSK" / "fairwindsk.ini"
            settings.parent.mkdir(parents=True)
            settings.write_text("[General]\nconfig=profiles/helm.json\n", encoding="utf-8")
            self.assertEqual(setup.config_path(), settings.parent / "profiles/helm.json")

    def test_merge_is_recursive_and_deterministic(self):
        base = {"main": {"language": "system", "windowMode": "windowed"}, "apps": [1]}
        merged = setup.merge(base, {"main": {"windowMode": "fullscreen"}})
        self.assertEqual(merged, {"main": {"language": "system", "windowMode": "fullscreen"}, "apps": [1]})

    def test_setup_creates_complete_config_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            default = root / "default.json"
            default.write_text(json.dumps({"main": {"language": "system"}, "apps": [1]}), encoding="utf-8")
            environment = {"XDG_CONFIG_HOME": str(root / "config"), "FAIRWINDSK_DEFAULT_CONFIG": str(default), "FAIRWINDSK_TEST_HALOS": "1"}
            with mock.patch.dict(os.environ, environment, clear=False), mock.patch.object(setup, "endpoint_ready", return_value=False):
                self.assertEqual(setup.setup(), 0)
                destination = setup.config_path()
                value = json.loads(destination.read_text(encoding="utf-8"))
                self.assertEqual(value["apps"], [1])
                self.assertEqual(value["main"]["windowMode"], "fullscreen")
                destination.write_text('{"operator":"preserved"}\n', encoding="utf-8")
                self.assertEqual(setup.setup(), 0)
                self.assertEqual(json.loads(destination.read_text(encoding="utf-8")), {"operator": "preserved"})
                self.assertTrue(setup.marker_path().is_file())

    def test_generic_linux_is_a_safe_noop(self):
        with tempfile.TemporaryDirectory() as directory, mock.patch.dict(os.environ, {"XDG_CONFIG_HOME": directory}, clear=False), mock.patch.object(setup, "is_halos", return_value=False):
            self.assertEqual(setup.setup(), 0)
            self.assertFalse(setup.config_path().exists())

    def test_atomic_failure_does_not_replace_existing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            destination = Path(directory) / "config.json"
            destination.write_text('{"old":true}\n', encoding="utf-8")
            with mock.patch.object(os, "replace", side_effect=OSError("simulated")):
                with self.assertRaises(OSError):
                    setup.atomic_write_json(destination, {"new": True})
            self.assertEqual(json.loads(destination.read_text(encoding="utf-8")), {"old": True})

    def test_diagnostic_url_drops_credentials_and_query(self):
        with tempfile.TemporaryDirectory() as directory, mock.patch.dict(os.environ, {"XDG_CONFIG_HOME": directory}, clear=False):
            setup.config_dir().mkdir(parents=True)
            setup.config_path().write_text(json.dumps({"connection": {"server": "https://user:secret@example.test:3443/path?token=secret"}}), encoding="utf-8")
            value = setup.safe_server()
            self.assertEqual(value, "https://example.test:3443/path")
            self.assertNotIn("secret", value)


class StartupTests(unittest.TestCase):
    def test_logged_url_drops_credentials_and_query(self):
        value = startup.safe_url("https://user:secret@example.test:3443/path?token=secret")
        self.assertEqual(value, "https://example.test:3443/path")

    def test_invalid_defaults_fall_back(self):
        with tempfile.TemporaryDirectory() as directory:
            defaults = Path(directory) / "fairwindsk"
            defaults.write_text("FAIRWINDSK_SERVER_WAIT_TIMEOUT=no\nFAIRWINDSK_SERVER_WAIT_INTERVAL=-1\nFAIRWINDSK_SERVER_SETTLE_DELAY=0\n", encoding="utf-8")
            with mock.patch.dict(os.environ, {"FAIRWINDSK_DEFAULTS_PATH": str(defaults)}, clear=False):
                self.assertEqual(startup.startup_timings(), (45, 2, 2))

    def test_no_server_skips_network_wait(self):
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(startup, "url_is_ready") as ready:
            startup.wait_for_signal_k_server(Path(directory) / "missing.json")
            ready.assert_not_called()

    def test_reachable_server_ends_wait(self):
        with tempfile.TemporaryDirectory() as directory:
            config = Path(directory) / "config.json"
            config.write_text(json.dumps({"connection": {"server": "http://localhost:3000"}}), encoding="utf-8")
            with mock.patch.object(startup, "url_is_ready", return_value=True), mock.patch.object(startup, "startup_timings", return_value=(5, 1, 1)), mock.patch.object(startup.time, "sleep") as sleep:
                startup.wait_for_signal_k_server(config)
                sleep.assert_called_once_with(1)


if __name__ == "__main__":
    unittest.main()
