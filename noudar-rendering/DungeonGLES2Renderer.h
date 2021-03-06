//
// Created by monty on 23/11/15.
//

#ifndef NOUDARRENDERING_DUNGEONRENDERER_H
#define NOUDARRENDERING_DUNGEONRENDERER_H


namespace odb {

    enum EFadeState {
        kNormal,
        kFadingIn,
        kFadingOut
    };

    enum EShaderType {
        kVertexShader,
        kFragmentShader,
    };

    using Shade = float;
    using AnimationList = std::map<int, std::tuple<glm::vec2, glm::vec2, long> >;
    using TextureId = int;

    static const long kAnimationLength = 500;

    class DungeonGLES2Renderer {

    private:
        CTilePropertyMap mTileProperties;
        std::map< VBORegisterId, VBORegister > mVBORegisters;
        std::map<std::string, ETextures > mTextureRegistry;

        void fetchShaderLocations();

        void setPerspective();

        void prepareShaderProgram();

        void clearBuffers();

        void resetTransformMatrices();

        void printVerboseDriverInformation();

        void createVBOs();

        void deleteVBOs();

        void drawGeometry(const unsigned int textureId, const int vertexVbo, const int indexVbo, int vertexCount,
                          const glm::mat4 &transform, float shade);

        unsigned int createProgram(const char *pVertexSource, const char *pFragmentSource);

        unsigned int loadShader(EShaderType shaderType, const char *pSource);

        glm::mat4 getSkyTransform(long offset);

        glm::mat4 getFloorTransform(glm::vec3 translation);

        glm::mat4 getBillboardTransform(glm::vec3 translation);

        glm::mat4 getCornerLeftFarTransform(glm::vec3 translation);

        glm::mat4 getCornerLeftNearTransform(glm::vec3 translation);

        glm::mat4 getCubeTransform(glm::vec3 translation);

        void consumeRenderingBatches(long animationTime);

        void produceRenderingBatches(const IntMap& map,const CharMap& actors, const IntMap& splats,const IntMap& lightmap,const IntMap& ids,
                                             const AnimationList& movingCharacters, long animationTime, const VisMap& visibilityMap);

        glm::vec3 transformToMapPosition(const glm::vec3 &pos);

        void initTileProperties();

    private:
        const static int kSkyTextureLength = 400;

        int vertexAttributePosition;
        int modelMatrixAttributePosition;
        int samplerUniformPosition;
        int textureCoordinatesAttributePosition;
        int projectionMatrixAttributePosition;
        unsigned int gProgram;
        unsigned int uView;
        unsigned int uMod;
        int mTurn = 0;
        glm::mat4 projectionMatrix;
        glm::mat4 mViewMatrix = glm::mat4( 1.0f );
        std::map<ETextures, std::vector<odb::VBORenderingJob>> batches;
        std::map<EActorsSnapshotElement, ETextures> mElementMap;
        std::map<std::string, std::shared_ptr<odb::Scene>> mMeshes;
        std::vector<std::shared_ptr<NativeBitmap>> mBitmaps;
        std::vector<std::shared_ptr<Texture>> mTextures;

        //fade state
        glm::vec4 mFadeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        int fadeUniform;
        EFadeState mFadeState = EFadeState::kNormal;

        //interaction
        bool mLongPressing = false;

        glm::vec2 mCursorPosition{0, 0};

        glm::vec3 mCurrentCharacterPosition;

        //camera
        glm::vec2 mCameraTarget;
        glm::vec2 cameraPosition;
        float *mEyeView = nullptr;

        float mAngleXZ = 0;
        float mAngleYZ = 0;

        int mCameraRotation = 0;
        int mRotationTarget = 0;

        //VBOs

        VBORegister mCubeVBO;
        VBORegister mBillboardVBO;
        VBORegister mCornerLeftFarVBO;
        VBORegister mCornerLeftNearVBO;
        VBORegister mFloorVBO;
        VBORegister mSkyVBO;

        const static float cubeVertices[16 * 5];
        const static unsigned short cubeIndices[6 * 4];

        static const float billboardVertices[20];
        static const unsigned short billboardIndices[6];

        static const float cornerLeftFarVertices[20];
        static const unsigned short cornerLeftFarIndices[6];

        static const float cornerLeftNearVertices[20];
        static const unsigned short cornerLeftNearIndices[6];

        static const float floorVertices[20];
        static const unsigned short floorIndices[6];

        static const float skyVertices[20];
        static const unsigned short skyIndices[6];


        float mPlayerHealth = 0.0f;
        VBORegisterId  mNullVBO = "null";
        TextureName mSkyBoxTextureName = "sky";
        TextureName mNullTexture = "null";
	    int mCameraId = 0;
    public:
        //basic bookeeping
        DungeonGLES2Renderer();

        ~DungeonGLES2Renderer();

        void setTurn( int turn );

        void setCameraId( int id );

        bool init(float w, float h, const std::string &vertexShader,
                  const std::string &fragmentShader);

        void setTexture(std::vector<std::shared_ptr<NativeBitmap>> textures);

        void render(const IntMap& map, const CharMap& actors, const IntMap& splats, const IntMap& lightmap,
                    const IntMap& ids, const AnimationList& movingCharacters, long animationTime, const VisMap& visibilityMap);

        void shutdown();

        void setMesh( std::shared_ptr<odb::Scene> mesh );

        void invalidateCachedBatches();

        void setTileProperties( CTilePropertyMap propertyMap );

        void setPerspectiveMatrix(float *perspectiveMatrix);

        void setClearColour(float r, float g, float b);

        VBORegister submitVBO( float* data, int vertices, unsigned short* indexData, unsigned int indices );

        //camera controls

        void updateCamera(long ms);

        VBORegister VBORegisterFrom( VBORegisterId id );
        ETextures textureIndexFrom( TextureName name );
        void rotateLeft();

        void rotateRight();

        bool isAnimating();

        void setEyeView(float *eyeView);

        void setCameraPosition(float x, float y);

        void setAngleXZ(float xz);

        void setAngleYZ(float yz);

        //fade
        void startFadingIn();

        void startFadingOut();

        void updateFadeState(long ms);

        //interactions
        void onLongPressingMove();

        bool isLongPressing();

        void setCursorPosition(int x, int y);

        void onReleasedLongPressingMove();

        void setPlayerHealth( float health );

        void resetCamera();

        float mWidth{ 0.0f };
        float mHeight{ 0.0f };
    };
}
#endif //NOUDARRENDERING_DUNGEONRENDERER_H
