#pragma once

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <chrono>
#include <cstdint>
#include <immer/array.hpp>
#include <immer/box.hpp>
#include <map>
#include <optional>
#include <thread>
#include <vector>

namespace wolf::core::input {

using namespace std::chrono_literals;

class VirtualDevice {
public:
  virtual std::vector<std::string> get_nodes() const = 0;

  virtual std::vector<std::map<std::string, std::string>> get_udev_events() const = 0;
  virtual std::vector<std::pair<std::string, /* filename */ std::vector<std::string> /* file rows */>>
  get_udev_hw_db_entries() const = 0;

  virtual ~VirtualDevice() = default;
};

/**
 * A virtual mouse device
 */
class Mouse : public VirtualDevice {
protected:
  typedef struct MouseState MouseState;

private:
  std::shared_ptr<MouseState> _state;

public:
  Mouse();
  Mouse(const Mouse &j) : _state(j._state) {}
  Mouse(Mouse &&j) : _state(std::move(j._state)) {}
  ~Mouse() override;

  std::vector<std::string> get_nodes() const override;

  std::vector<std::map<std::string, std::string>> get_udev_events() const override;
  std::vector<std::pair<std::string, std::vector<std::string>>> get_udev_hw_db_entries() const override;

  void move(int delta_x, int delta_y);

  void move_abs(int x, int y, int screen_width, int screen_height);

  enum MOUSE_BUTTON {
    LEFT,
    MIDDLE,
    RIGHT,
    SIDE,
    EXTRA
  };

  void press(MOUSE_BUTTON button);

  void release(MOUSE_BUTTON button);

  /**
   *
   * A value that is a fraction of ±120 indicates a wheel movement less than
   * one logical click, a caller should either scroll by the respective
   * fraction of the normal scroll distance or accumulate that value until a
   * multiple of 120 is reached.
   *
   * The magic number 120 originates from the
   * <a href="http://download.microsoft.com/download/b/d/1/bd1f7ef4-7d72-419e-bc5c-9f79ad7bb66e/wheel.docx">
   * Windows Vista Mouse Wheel design document
   * </a>.
   *
   * Positive numbers will scroll down, negative numbers will scroll up
   *
   * @param high_res_distance The distance in high resolution
   */
  void vertical_scroll(int high_res_distance);

  /**
   *
   * A value that is a fraction of ±120 indicates a wheel movement less than
   * one logical click, a caller should either scroll by the respective
   * fraction of the normal scroll distance or accumulate that value until a
   * multiple of 120 is reached.
   *
   * The magic number 120 originates from the
   * <a href="http://download.microsoft.com/download/b/d/1/bd1f7ef4-7d72-419e-bc5c-9f79ad7bb66e/wheel.docx">
   * Windows Vista Mouse Wheel design document
   * </a>.
   *
   * Positive numbers will scroll right, negative numbers will scroll left
   *
   * @param high_res_distance The distance in high resolution
   */
  void horizontal_scroll(int high_res_distance);
};

/**
 * A virtual trackpad
 *
 * implements a pure multi-touch touchpad as defined in libinput
 * https://wayland.freedesktop.org/libinput/doc/latest/touchpads.html
 */
class Trackpad : public VirtualDevice {
protected:
  typedef struct TrackpadState TrackpadState;

private:
  std::shared_ptr<TrackpadState> _state;

public:
  Trackpad();
  Trackpad(const Trackpad &j) : _state(j._state) {}
  Trackpad(Trackpad &&j) : _state(std::move(j._state)) {}
  ~Trackpad() override;

  std::vector<std::string> get_nodes() const override;

  std::vector<std::map<std::string, std::string>> get_udev_events() const override;
  std::vector<std::pair<std::string, std::vector<std::string>>> get_udev_hw_db_entries() const override;

  /**
   * We expect (x,y) to be in the range [0.0, 1.0]; x and y values are normalised device coordinates
   * from the top-left corner (0.0, 0.0) to bottom-right corner (1.0, 1.0)
   *
   * @param finger_nr
   * @param pressure A value between 0 and 1
   */
  void place_finger(int finger_nr, float x, float y, float pressure);

  void release_finger(int finger_nr);

  void set_left_btn(bool pressed);
};

/**
 * A virtual touchscreen
 */
class TouchScreen : public VirtualDevice {
protected:
  typedef struct TouchScreenState TouchScreenState;

private:
  std::shared_ptr<TouchScreenState> _state;

public:
  TouchScreen();
  TouchScreen(const TouchScreen &j) : _state(j._state) {}
  TouchScreen(TouchScreen &&j) : _state(std::move(j._state)) {}
  ~TouchScreen() override;

  std::vector<std::string> get_nodes() const override;

  std::vector<std::map<std::string, std::string>> get_udev_events() const override;
  std::vector<std::pair<std::string, std::vector<std::string>>> get_udev_hw_db_entries() const override;

  /**
   * We expect (x,y) to be in the range [0.0, 1.0]; x and y values are normalised device coordinates
   * from the top-left corner (0.0, 0.0) to bottom-right corner (1.0, 1.0)
   *
   * @param finger_nr
   * @param pressure A value between 0 and 1
   */
  void place_finger(int finger_nr, float x, float y, float pressure);

  void release_finger(int finger_nr);
};

/**
 * A virtual pen tablet
 *
 * implements a pen tablet as defined in libinput
 * https://wayland.freedesktop.org/libinput/doc/latest/tablet-support.html
 */
class PenTablet : public VirtualDevice {
protected:
  typedef struct PenTabletState PenTabletState;

private:
  std::shared_ptr<PenTabletState> _state;

public:
  PenTablet();
  PenTablet(const PenTablet &j) : _state(j._state) {}
  PenTablet(PenTablet &&j) : _state(std::move(j._state)) {}
  ~PenTablet() override;

  std::vector<std::string> get_nodes() const override;

  std::vector<std::map<std::string, std::string>> get_udev_events() const override;
  std::vector<std::pair<std::string, std::vector<std::string>>> get_udev_hw_db_entries() const override;

  enum TOOL_TYPE {
    PEN,
    ERASER,
    BRUSH,
    PENCIL,
    AIRBRUSH,
    TOUCH,
    SAME_AS_BEFORE /* Real devices don't need to report the tool type when it's still the same */
  };

  enum BTN_TYPE {
    PRIMARY,
    SECONDARY,
    TERTIARY
  };

  /**
   * x,y,pressure and distance should be normalized in the range [0.0, 1.0].
   * Passing a negative value will discard that value; this is used to report pressure instead of distance
   * (they should never be both positive).
   *
   * tilt_x and tilt_y are in the range [-90.0, 90.0] degrees.
   *
   * Refer to the libinput docs to better understand what each param means:
   * https://wayland.freedesktop.org/libinput/doc/latest/tablet-support.html#special-axes-on-tablet-tools
   */
  void place_tool(TOOL_TYPE tool_type, float x, float y, float pressure, float distance, float tilt_x, float tilt_y);

  void set_btn(BTN_TYPE btn, bool pressed);
};

/**
 * A virtual keyboard device
 *
 * Key codes are Win32 Virtual Key (VK) codes
 * Users of this class can expect that if a key is pressed, it'll be re-pressed every
 * time_repress_key until it's released.
 */
class Keyboard : public VirtualDevice {
protected:
  typedef struct KeyboardState KeyboardState;

private:
  std::shared_ptr<KeyboardState> _state;

public:
  explicit Keyboard(std::chrono::milliseconds timeout_repress_key = 50ms);

  Keyboard(const Keyboard &j) : _state(j._state) {}
  Keyboard(Keyboard &&j) : _state(std::move(j._state)) {}
  ~Keyboard() override;

  std::vector<std::string> get_nodes() const override;

  std::vector<std::map<std::string, std::string>> get_udev_events() const override;
  std::vector<std::pair<std::string, std::vector<std::string>>> get_udev_hw_db_entries() const override;

  void press(short key_code);

  void release(short key_code);

  /**
   * Here we receive a single UTF-8 encoded char at a time,
   * the trick is to convert it to UTF-32 then send CTRL+SHIFT+U+<HEXCODE> in order to produce any
   * unicode character, see: https://en.wikipedia.org/wiki/Unicode_input
   *
   * ex:
   * - when receiving UTF-8 [0xF0 0x9F 0x92 0xA9] (which is '💩')
   * - we'll convert it to UTF-32 [0x1F4A9]
   * - then type: CTRL+SHIFT+U+1F4A9
   * see the conversion at: https://www.compart.com/en/unicode/U+1F4A9
   */
  void paste_utf(const std::basic_string<char32_t> &utf32);
};

/**
 * An abstraction on top of a virtual joypad
 * In order to support callbacks (ex: on_rumble()) this will create a new thread for listening for such events
 */
class Joypad : public VirtualDevice {
protected:
  typedef struct JoypadState JoypadState;

private:
  std::shared_ptr<JoypadState> _state;

public:
  enum CONTROLLER_TYPE : uint8_t {
    UNKNOWN = 0x00,
    XBOX = 0x01,
    PS = 0x02,
    NINTENDO = 0x03
  };

  enum CONTROLLER_CAPABILITIES : uint8_t {
    ANALOG_TRIGGERS = 0x01,
    RUMBLE = 0x02,
    TRIGGER_RUMBLE = 0x04,
    TOUCHPAD = 0x08,
    ACCELEROMETER = 0x10,
    GYRO = 0x20,
    BATTERY = 0x40,
    RGB_LED = 0x80
  };

  Joypad(CONTROLLER_TYPE type, uint8_t capabilities);

  Joypad(const Joypad &j) : _state(j._state) {}
  Joypad(Joypad &&j) : _state(std::move(j._state)) {}
  ~Joypad() override;

  std::vector<std::string> get_nodes() const override;

  std::vector<std::map<std::string, std::string>> get_udev_events() const override;
  std::vector<std::pair<std::string, std::vector<std::string>>> get_udev_hw_db_entries() const override;

  enum CONTROLLER_BTN : int {
    DPAD_UP = 0x0001,
    DPAD_DOWN = 0x0002,
    DPAD_LEFT = 0x0004,
    DPAD_RIGHT = 0x0008,

    START = 0x0010,
    BACK = 0x0020,
    HOME = 0x0400,

    LEFT_STICK = 0x0040,
    RIGHT_STICK = 0x0080,
    LEFT_BUTTON = 0x0100,
    RIGHT_BUTTON = 0x0200,

    SPECIAL_FLAG = 0x0400,
    PADDLE1_FLAG = 0x010000,
    PADDLE2_FLAG = 0x020000,
    PADDLE3_FLAG = 0x040000,
    PADDLE4_FLAG = 0x080000,
    TOUCHPAD_FLAG = 0x100000, // Touchpad buttons on Sony controllers
    MISC_FLAG = 0x200000,     // Share/Mic/Capture/Mute buttons on various controllers

    A = 0x1000,
    B = 0x2000,
    X = 0x4000,
    Y = 0x8000
  };

  /**
   * Given the nature of joypads we (might) have to simultaneously press and release multiple buttons.
   * In order to implement this, you can pass a single short: button_flags which represent the currently pressed
   * buttons in the joypad.
   * This class will keep an internal state of the joypad and will automatically release buttons that are no
   * longer pressed.
   *
   * Example: previous state had `DPAD_UP` and `A` -> user release `A` -> new state only has `DPAD_UP`
   */
  void set_pressed_buttons(int newly_pressed);

  void set_triggers(int16_t left, int16_t right);

  enum STICK_POSITION {
    RS,
    LS
  };

  void set_stick(STICK_POSITION stick_type, short x, short y);

  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback);

  /**
   * @see: Trackpad->place_finger
   */
  void touchpad_place_finger(int finger_nr, float x, float y, float pressure);

  /**
   * @see: Trackpad->release_finger
   */
  void touchpad_release_finger(int finger_nr);

  enum MOTION_TYPE : uint8_t {
    ACCELERATION = 0x01,
    GYROSCOPE = 0x02
  };

  void set_motion(MOTION_TYPE type, float x, float y, float z);

  enum BATTERY_STATE : uint8_t {
    NOT_KNOWN = 0x00,
    NOT_PRESENT = 0x01,
    DISCHARGING = 0x02,
    CHARGHING = 0x03,
    NOT_CHARGING = 0x04,
    FULL = 0x05
  };

  void set_battery(BATTERY_STATE state, int percentage);

  void set_on_led(const std::function<void(int r, int g, int b)> &callback);
};
} // namespace wolf::core::input
