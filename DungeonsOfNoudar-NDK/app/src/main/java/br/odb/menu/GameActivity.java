package br.odb.menu;

import android.app.Activity;
import android.os.Bundle;

import br.odb.ControllerHelper;
import br.odb.GameViewGLES2;

public class GameActivity extends Activity {

	public static final String USE_VR = "use-vr";

	private ControllerHelper mControllerHelper;
	private GameViewGLES2 view;
	private boolean mHaveController;

	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);

		mControllerHelper = new ControllerHelper( this );

		configureUiForInputDevice();

		view = new GameViewGLES2( this );
		setContentView( view );
		view.init(this, 0, mHaveController);
	}

	@Override
	protected void onPause() {
		super.onPause();

		view.onPause();
		view.onDestroy();
	}

	@Override
	protected void onResume() {
		super.onResume();

		view.onCreate(getAssets());
		view.onResume();

	}
	//presentation and interaction

	private void configureUiForInputDevice() {
		mHaveController = mControllerHelper.hasGamepad() || mControllerHelper.hasPhysicalKeyboard();
	}
}
