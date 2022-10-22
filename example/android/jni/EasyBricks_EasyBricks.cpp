
#include "EasyBricks_EasyBricks.h"
#include <set>
#include "facade/game_system.h"

std::map<int, std::shared_ptr<GameSystem>> id_system_map;

int GetJObjectID(JNIEnv *jni, jobject obj) {
  jclass clazz = jni->FindClass("EasyBricks/EasyBricks");
  jfieldID id_field = jni->GetFieldID(clazz, "id", "I");
  jint id = jni->GetIntField(obj, id_field);
  return id;
}

void SetJObjectID(JNIEnv *jni, jobject obj, int id) {
  jclass clazz = jni->FindClass("EasyBricks/EasyBricks");
  jfieldID id_field = jni->GetFieldID(clazz, "id", "I");
  jni->SetIntField(obj, id_field, id);
}

// std::vector<PlayOperation> -> jobjectArray
jobjectArray PlayOperationCppToJava(
    JNIEnv *jni, const std::vector<PlayOperation> &play_ops) {
  int size = play_ops.size();
  jclass clazz = jni->FindClass("EasyBricks/PlayOperation");
  jmethodID id_method = jni->GetMethodID(clazz, "<init>", "()V");
  jobject default_obj = jni->NewObject(clazz, id_method);
  jobjectArray ret = jni->NewObjectArray(size, clazz, default_obj);

  for (int idx = 0; idx < size; ++idx) {
    jfieldID type_field = jni->GetFieldID(clazz, "type", "I");
    jfieldID x_field = jni->GetFieldID(clazz, "x", "I");
    jfieldID y_field = jni->GetFieldID(clazz, "y", "I");
    jfieldID sleep_field = jni->GetFieldID(clazz, "sleep_ms", "I");

    jobject obj = jni->NewObject(clazz, id_method);
    jni->SetIntField(obj, type_field, int(play_ops[idx].type));
    if (play_ops[idx].type == PlayOperation::Type::SCREEN_CLICK) {
      jni->SetIntField(obj, x_field, play_ops[idx].click.x);
      jni->SetIntField(obj, y_field, play_ops[idx].click.y);
    } else if (play_ops[idx].type == PlayOperation::Type::SLEEP) {
      jni->SetIntField(obj, sleep_field, play_ops[idx].sleep_ms);
    } else if (play_ops[idx].type == PlayOperation::Type::OVER) {
    }
    jni->SetObjectArrayElement(ret, idx, obj);
  }
  return ret;
}

// std::vector<std::string> -> jobjectArray
jobjectArray VectorStringCppToJava(JNIEnv *jni,
                                   const std::vector<std::string> &strs) {
  jclass str_class = jni->FindClass("java/lang/String");
  jobjectArray ret = jni->NewObjectArray(strs.size(), str_class, 0);
  for (int idx = 0; idx < strs.size(); ++idx) {
    jstring jstr = jni->NewStringUTF(strs[idx].c_str());
    jni->SetObjectArrayElement(ret, idx, jstr);
  }
  return ret;
}

// Init
jboolean Java_EasyBricks_EasyBricks_Init(JNIEnv *jni, jobject obj) {
  int id = GetJObjectID(jni, obj);
  return id_system_map[id]->Init();
}

// Players
jobjectArray Java_EasyBricks_EasyBricks_Players(JNIEnv *jni, jobject obj) {
  int id = GetJObjectID(jni, obj);
  std::shared_ptr<GameSystem> system = id_system_map[id];
  std::vector<std::string> players = system->Players();
  return VectorStringCppToJava(jni, players);
}

// Modes
jobjectArray Java_EasyBricks_EasyBricks_Modes(JNIEnv *jni, jobject obj,
                                              jstring jstr) {
  int id = GetJObjectID(jni, obj);
  std::shared_ptr<GameSystem> system = id_system_map[id];
  const char *s = jni->GetStringUTFChars(jstr, nullptr);
  std::string player(s);
  jni->ReleaseStringUTFChars(jstr, s);
  std::vector<std::string> modes = system->Modes(player);
  return VectorStringCppToJava(jni, modes);
}

// Player
jstring Java_EasyBricks_EasyBricks_Player(JNIEnv *jni, jobject obj) {
  int id = GetJObjectID(jni, obj);
  std::shared_ptr<GameSystem> system = id_system_map[id];
  std::string player = system->Player();
  jstring jstr = jni->NewStringUTF(player.c_str());
  return jstr;
}

// Mode
jstring Java_EasyBricks_EasyBricks_Mode(JNIEnv *jni, jobject obj) {
  int id = GetJObjectID(jni, obj);
  std::shared_ptr<GameSystem> system = id_system_map[id];
  std::string mode = system->Mode();
  jstring jstr = jni->NewStringUTF(mode.c_str());
  return jstr;
}

// SetPlayMode
jboolean Java_EasyBricks_EasyBricks_SetPlayMode(JNIEnv *jni, jobject obj,
                                                jstring player, jstring mode) {
  const char *player_p = jni->GetStringUTFChars(player, nullptr);
  const char *mode_p = jni->GetStringUTFChars(mode, nullptr);
  std::string player_name(player_p);
  std::string mode_name(mode_p);
  jni->ReleaseStringUTFChars(player, player_p);
  jni->ReleaseStringUTFChars(mode, mode_p);
  int id = GetJObjectID(jni, obj);
  std::shared_ptr<GameSystem> system = id_system_map[id];
  return system->SetPlayMode(player_name, mode_name);
}

// InputImage
jobjectArray Java_EasyBricks_EasyBricks_InputImage(JNIEnv *jni, jobject obj,
                                                   jint format,
                                                   jbyteArray arr) {
  int id = GetJObjectID(jni, obj);
  std::shared_ptr<GameSystem> system = id_system_map[id];
  int size = jni->GetArrayLength(arr);
  uint8_t *buffer = (uint8_t *)malloc(size);
  jni->GetByteArrayRegion(arr, 0, size, (jbyte *)buffer);
  std::vector<PlayOperation> play_ops =
      system->InputImage(ImageFormat(format), buffer, size);
  return PlayOperationCppToJava(jni, play_ops);
}

// InputRawImage
jobjectArray Java_EasyBricks_EasyBricks_InputRawImage(JNIEnv *jni, jobject obj,
                                                      jint format, jint width,
                                                      jint height,
                                                      jbyteArray arr) {
  int id = GetJObjectID(jni, obj);
  std::shared_ptr<GameSystem> system = id_system_map[id];
  int size = jni->GetArrayLength(arr);
  uint8_t *buffer = (uint8_t *)malloc(size);
  jni->GetByteArrayRegion(arr, 0, size, (jbyte *)buffer);
  std::vector<PlayOperation> play_ops =
      system->InputRawImage(ImageFormat(format), width, height, buffer, size);
  return PlayOperationCppToJava(jni, play_ops);
}

void Java_EasyBricks_EasyBricks_finalize(JNIEnv *jni, jobject obj) {
  id_system_map.clear();
}

// Create
void Java_EasyBricks_EasyBricks_Create(JNIEnv *jni, jobject obj,
                                       jstring config_fname) {
  int new_id = 1;
  for (auto [id, _] : id_system_map) {
    new_id = std::max(new_id, id + 1);
  }
  SetJObjectID(jni, obj, new_id);
  const char *p = jni->GetStringUTFChars(config_fname, nullptr);
  std::string fname(p);
  jni->ReleaseStringUTFChars(config_fname, p);
  id_system_map[new_id] = std::make_shared<GameSystem>(fname);
}
