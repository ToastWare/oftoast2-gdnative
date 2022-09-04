#include <gdnative_api_struct.gen.h>
#include <string.h>
#include <curl/curl.h>

typedef enum {
	GDDL_OK,
	GDDL_CURL,
	GDDL_FILE,
	GDDL_BAD_ARGS
} gddl_err_code;

typedef struct user_data_struct {
	//CURL stuff
	CURL *curl;
	CURLcode res;
	//Our error
	gddl_err_code err;
} user_data_struct;

//more curl stuff
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}


// GDNative supports a large collection of functions for calling back
// into the main Godot executable. In order for your module to have
// access to these functions, GDNative provides your application with
// a struct containing pointers to all these functions.
const godot_gdnative_core_api_struct *api = NULL;
const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;

// These are forward declarations for the functions we'll be implementing
// for our object. A constructor and destructor are both necessary.
GDCALLINGCONV void *simple_constructor(godot_object *p_instance, void *p_method_data);
GDCALLINGCONV void simple_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data);
//godot_variant simple_get_data(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);

godot_variant gddl_download_file(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant gddl_get_error(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);

// `gdnative_init` is a function that initializes our dynamic library.
// Godot will give it a pointer to a structure that contains various bits of
// information we may find useful among which the pointers to our API structures.
void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *p_options) {
	api = p_options->api_struct;

	// Find NativeScript extensions.
	for (int i = 0; i < api->num_extensions; i++) {
		switch (api->extensions[i]->type) {
			case GDNATIVE_EXT_NATIVESCRIPT: {
				nativescript_api = (godot_gdnative_ext_nativescript_api_struct *)api->extensions[i];
			}; break;
			default:
				break;
		};
	};
}

// `gdnative_terminate` which is called before the library is unloaded.
// Godot will unload the library when no object uses it anymore.
void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *p_options) {
	api = NULL;
	nativescript_api = NULL;
}

// `nativescript_init` is the most important function. Godot calls
// this function as part of loading a GDNative library and communicates
// back to the engine what objects we make available.
void GDN_EXPORT godot_nativescript_init(void *p_handle) {
	godot_instance_create_func create = { NULL, NULL, NULL };
	create.create_func = &simple_constructor;

	godot_instance_destroy_func destroy = { NULL, NULL, NULL };
	destroy.destroy_func = &simple_destructor;

	// We first tell the engine which classes are implemented by calling this.
	// * The first parameter here is the handle pointer given to us.
	// * The second is the name of our object class.
	// * The third is the type of object in Godot that we 'inherit' from;
	//   this is not true inheritance but it's close enough.
	// * Finally, the fourth and fifth parameters are descriptions
	//   for our constructor and destructor, respectively.
	nativescript_api->godot_nativescript_register_class(p_handle, "GDDL", "Reference", create, destroy);

	godot_instance_method download_file = { NULL, NULL, NULL };
	godot_instance_method get_error = { NULL, NULL, NULL };
	download_file.method = &gddl_download_file;
	get_error.method = &gddl_get_error;

	godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

	// We then tell Godot about our methods by calling this for each
	// method of our class. In our case, this is just `get_data`.
	// * Our first parameter is yet again our handle pointer.
	// * The second is again the name of the object class we're registering.
	// * The third is the name of our function as it will be known to GDScript.
	// * The fourth is our attributes setting (see godot_method_rpc_mode enum in
	//   `godot_headers/nativescript/godot_nativescript.h` for possible values).
	// * The fifth and final parameter is a description of which function
	//   to call when the method gets called.
	//nativescript_api->godot_nativescript_register_method(p_handle, "SIMPLE", "get_data", attributes, get_data);
	nativescript_api->godot_nativescript_register_method(p_handle, "GDDL", "download_file", attributes, download_file);
	nativescript_api->godot_nativescript_register_method(p_handle, "GDDL", "get_error", attributes, get_error);

}

// In our constructor, allocate memory for our structure and fill
// it with some data. Note that we use Godot's memory functions
// so the memory gets tracked and then return the pointer to
// our new structure. This pointer will act as our instance
// identifier in case multiple objects are instantiated.
GDCALLINGCONV void *simple_constructor(godot_object *p_instance, void *p_method_data) {
	user_data_struct *user_data = api->godot_alloc(sizeof(user_data_struct));
	//strcpy(user_data->data, "World from GDNative!");
	user_data->curl = curl_easy_init();

	curl_easy_setopt(user_data->curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(user_data->curl, CURLOPT_WRITEFUNCTION, write_data);

	user_data->res = CURLE_OK;
	user_data->err = GDDL_OK;

	return user_data;
}

// The destructor is called when Godot is done with our
// object and we free our instances' member data.
GDCALLINGCONV void simple_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data) {
	user_data_struct *user_data = (user_data_struct *)p_user_data;
	curl_easy_cleanup(user_data->curl);
	api->godot_free(p_user_data);
}

// Data is always sent and returned as variants so in order to
// return our data, which is a string, we first need to convert
// our C string to a Godot string object, and then copy that
// string object into the variant we are returning.
godot_variant simple_get_data(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_string data;
	godot_variant ret;
	user_data_struct *user_data = (user_data_struct *)p_user_data;

	api->godot_string_new(&data);
	//api->godot_string_parse_utf8(&data, user_data->data);
	api->godot_variant_new_string(&ret, &data);
	api->godot_string_destroy(&data);

	return ret;
}

godot_variant gddl_download_file(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	user_data_struct *user_data = (user_data_struct *)p_user_data;
	godot_variant ret;

	api->godot_variant_new_nil(&ret);

	if (p_num_args < 2) {
		api->godot_variant_new_bool(&ret, false);
		user_data->err = GDDL_BAD_ARGS;
		return ret;
	}
	godot_string gd_url, gd_path;
	
	
	gd_url = api->godot_variant_as_string(p_args[0]);
	godot_char_string url_char_string = api->godot_string_utf8(&gd_url);
	const char *url = api->godot_char_string_get_data(&url_char_string);

	gd_path = api->godot_variant_as_string(p_args[1]);
	godot_char_string path_char_string = api->godot_string_utf8(&gd_path);
	const char *path = api->godot_char_string_get_data(&path_char_string);

	FILE* f = fopen(path, "wb");
	if (!f) {
		api->godot_variant_new_bool(&ret, false);
		user_data->err = GDDL_FILE;
		return ret;
	}

	curl_easy_setopt(user_data->curl, CURLOPT_URL, url);
	curl_easy_setopt(user_data->curl, CURLOPT_WRITEDATA, f);

	user_data->res = curl_easy_perform(user_data->curl);
	fclose(f);
	if (user_data->res != CURLE_OK) {
		api->godot_variant_new_bool(&ret, false);
		user_data->err = GDDL_CURL;
		return ret;
	}

	api->godot_char_string_destroy(&url_char_string);
	api->godot_char_string_destroy(&path_char_string);

	api->godot_variant_new_bool(&ret, true);
	user_data->err = GDDL_OK;
	return ret;
}

godot_variant gddl_get_error(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	user_data_struct *user_data = (user_data_struct *)p_user_data;
	godot_variant ret;
	godot_string str;

	api->godot_string_new(&str);
	
	switch(user_data->err) {
		case GDDL_OK:
			api->godot_string_parse_utf8(&str, "No error.");
			break;
		case GDDL_CURL:
			api->godot_string_parse_utf8(&str, curl_easy_strerror(user_data->res));
			break;
		case GDDL_FILE:
			api->godot_string_parse_utf8(&str, "Error opening file to write.");
			break;
		case GDDL_BAD_ARGS:
			api->godot_string_parse_utf8(&str, "Invalid arguments.");
			break;
		default:
			char buf[256];
			sprintf(buf, "Unknown error. Error code: %d", user_data->err);
			api->godot_string_parse_utf8(&str, buf);
			break;
	}

	api->godot_variant_new_string(&ret, &str);

	return ret;
}