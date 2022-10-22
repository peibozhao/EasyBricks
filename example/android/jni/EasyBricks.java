
package EasyBricks;

public class EasyBricks {
  public EasyBricks(String config_fname) {
    Create(config_fname);
  }

  public native boolean Init();

  public native String []Players();

  public native String []Modes(String player);

  public native String Player();

  public native String Mode();

  public native boolean SetPlayMode(String player, String mode);

  public native PlayOperation []InputImage(int format, byte []buffer);

  public native PlayOperation []InputRawImage(int format, int width, int height, byte []buffer);

  protected native void finalize() throws Throwable;

  private native void Create(String config_fname);

  private int id;
};
