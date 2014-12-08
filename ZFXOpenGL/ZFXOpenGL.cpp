#include "ZFXOpenGL.h"
#include <stdexcept>
#include <math.h>
#include <gl\glew.h>

bool g_bLF = false;

ZFXOpenGL::ZFXOpenGL()
{
	m_mView3D.Identity();
	m_mWorld3D.Identity();
}


ZFXOpenGL::~ZFXOpenGL()
{
}

void ZFXOpenGL::SetClippingPlanes(float fNear, float fFar)
{
	GLdouble fNearplane[] = { 0, 0, 1, fNear };
	GLdouble fFarplane[] = { 0, 0, 1, fFar };

	glPushMatrix();

	// fNear clip
	glClipPlane(GL_CLIP_PLANE0, fNearplane);
	glEnable(GL_CLIP_PLANE0);

	// fFar clip
	glClipPlane(GL_CLIP_PLANE1, fFarplane);
	glEnable(GL_CLIP_PLANE1);

	glPopMatrix();

	m_fNear = fNear;
	m_fFar = fFar;

	if (m_fNear <= 0.0f)
		m_fNear = 0.01f;

	if (m_fFar <= 1.0f)
		m_fFar = 1.00f;

	if (m_fNear >= m_fFar) {
		m_fNear = m_fFar;
		m_fFar = m_fNear + 1.0f;
	}

	// change 2D projection and view
	Prepare2D();

	// change orthogonal projection
	float Q = 1.0f / (m_fFar - m_fNear);
	float X = m_fNear / (m_fNear - m_fFar);
	m_mProjO[0]._33 = m_mProjO[1]._33 = Q;
	m_mProjO[2]._33 = m_mProjO[3]._33 = Q;
	m_mProjO[0]._43 = m_mProjO[1]._43 = X;
	m_mProjO[2]._43 = m_mProjO[3]._43 = X;

	// change perspective projection
	Q *= m_fFar;
	X = -Q * m_fNear;
	m_mProjP[0]._33 = m_mProjP[1]._33 = Q;
	m_mProjP[2]._33 = m_mProjP[3]._33 = Q;
	m_mProjP[0]._43 = m_mProjP[1]._43 = X;
	m_mProjP[2]._43 = m_mProjP[3]._43 = X;
}

HRESULT ZFXOpenGL::SetMode(ZFXENGINEMODE mode, int nStage)
{
	if ((nStage > 3) || (nStage < 0)) nStage = 0;
	if (m_Mode != mode)
		m_Mode = mode;

	throw std::logic_error("flush all vertext cache");

	m_nStage = nStage;

	DWORD x = m_VP[nStage].X;
	DWORD y = m_VP[nStage].Y;
	DWORD width = m_VP[nStage].Width;
	DWORD height = m_VP[nStage].Height;

	glViewport(x, y, width, height);

	GLfloat mat[16];

	if (mode == EMD_TWOD)
	{
		if (!m_bUseShaders)
		{
			glMatrixMode(GL_PROJECTION);
			MakeGLMatrix(mat, m_mProj2D);
			glLoadMatrixf(mat);
			MakeGLMatrix(mat, m_mView2D * m_mWorld2D);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(mat);
		}
	}
	else
	{
		if (!m_bUseShaders)
		{
			glMatrixMode(GL_MODELVIEW);
			MakeGLMatrix(mat, m_mView3D * m_mWorld3D);
			glLoadMatrixf(mat);

			if (m_Mode = EMD_PERSPECTIVE)
			{
				glMatrixMode(GL_PROJECTION);
				MakeGLMatrix(mat, m_mProjP[nStage]);
				glLoadMatrixf(mat);
			}
			else
			{
				glMatrixMode(GL_PROJECTION);
				MakeGLMatrix(mat, m_mProjO[nStage]);
				glLoadMatrixf(mat);
			}
		}
		CalcViewProjMatrix();
		CalcWorldViewProjMatrix();
	}

	return ZFX_OK;
	///throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::SetOrthoScale(float fScale, int nStage)
{
	float fW = ((float)m_dwWidth) / m_dwHeight * fScale;
	float fH = fScale;
	memset(&m_mProjO[nStage], 0, sizeof(ZFXMatrix));
	m_mProjO[nStage]._11 = 2.0f / fW;
	m_mProjO[nStage]._22 = 2.0f / fH;
	m_mProjO[nStage]._33 = 1.0f / (m_fFar - m_fNear);
	m_mProjO[nStage]._43 = m_fNear / (m_fNear - m_fFar);
	m_mProjO[nStage]._44 = 1.0f;
	//throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::InitStage(float fFov, ZFXVIEWPORT* pView, int nStage)
{
	float fAspect;
	bool  bOwnRect = false;

	if (!pView)
	{
		ZFXVIEWPORT vpOwn = { 0, 0, m_dwWidth, m_dwHeight };
		memcpy(&m_VP[nStage], &vpOwn, sizeof(RECT));
	}
	else
		memcpy(&m_VP[nStage], pView, sizeof(RECT));

	if ((nStage > 3) || (nStage < 0)) nStage = 0;

	fAspect = ((float)(m_VP[nStage].Height)) / (m_VP[nStage].Width);

	// PERSPECTIVE PROJEKTION MATRIX
	if (FAILED(this->CalcPerspProjMatrix(fFov, fAspect, &m_mProjP[nStage])))
		return ZFX_FAIL;

	// ORTHOGONAL PROJECTION MATRIX
	memset(&m_mProjO[nStage], 0, sizeof(float) * 16);
	m_mProjO[nStage]._11 = 2.0f / m_VP[nStage].Width;
	m_mProjO[nStage]._22 = 2.0f / m_VP[nStage].Height;
	m_mProjO[nStage]._33 = 1.0f / (m_fFar - m_fNear);

	m_mProjO[nStage]._43 = m_fNear / (m_fNear - m_fFar);
	m_mProjO[nStage]._44 = 1.0f;

	return ZFX_OK;
	// throw std::logic_error("The method or operation is not implemented.");
}

// 获取构成平截头体的六个面
HRESULT ZFXOpenGL::GetFrustrum(ZFXPlane* p)
{
	// left plane
	p[0].m_vcN.x = -(m_mViewProj._14 + m_mViewProj._11);
	p[0].m_vcN.y = -(m_mViewProj._24 + m_mViewProj._21);
	p[0].m_vcN.z = -(m_mViewProj._34 + m_mViewProj._31);
	p[0].m_fD = -(m_mViewProj._44 + m_mViewProj._41);

	// right plane
	p[1].m_vcN.x = -(m_mViewProj._14 - m_mViewProj._11);
	p[1].m_vcN.y = -(m_mViewProj._24 - m_mViewProj._21);
	p[1].m_vcN.z = -(m_mViewProj._34 - m_mViewProj._31);
	p[1].m_fD = -(m_mViewProj._44 - m_mViewProj._41);

	// top plane
	p[2].m_vcN.x = -(m_mViewProj._14 - m_mViewProj._12);
	p[2].m_vcN.y = -(m_mViewProj._24 - m_mViewProj._22);
	p[2].m_vcN.z = -(m_mViewProj._34 - m_mViewProj._32);
	p[2].m_fD = -(m_mViewProj._44 - m_mViewProj._42);

	// bottom plane
	p[3].m_vcN.x = -(m_mViewProj._14 + m_mViewProj._12);
	p[3].m_vcN.y = -(m_mViewProj._24 + m_mViewProj._22);
	p[3].m_vcN.z = -(m_mViewProj._34 + m_mViewProj._32);
	p[3].m_fD = -(m_mViewProj._44 + m_mViewProj._42);

	// near plane
	p[4].m_vcN.x = -m_mViewProj._13;
	p[4].m_vcN.y = -m_mViewProj._23;
	p[4].m_vcN.z = -m_mViewProj._33;
	p[4].m_fD = -m_mViewProj._43;

	// far plane
	p[5].m_vcN.x = -(m_mViewProj._14 - m_mViewProj._13);
	p[5].m_vcN.y = -(m_mViewProj._24 - m_mViewProj._23);
	p[5].m_vcN.z = -(m_mViewProj._34 - m_mViewProj._33);
	p[5].m_fD = -(m_mViewProj._44 - m_mViewProj._43);

	// normalize frustrum normals
	for (int i = 0; i < 6; i++) {
		float fL = p[i].m_vcN.GetLength();
		p[i].m_vcN /= fL;
		p[i].m_fD /= fL;
	}

	return ZFX_OK;
}

// 屏幕鼠标映射到三维射线
void ZFXOpenGL::Transform2Dto3D(const POINT &pt, ZFXVector *vcOrig, ZFXVector *vcDir)
{
	ZFXMatrix *pView = NULL, *pProj = NULL;
	ZFXMatrix mInvView;
	ZFXVector vcS;
	DWORD dwWidth, dwHeight;

	// if 2D mode
	if (m_Mode == EMD_TWOD) 
	{
		dwWidth = m_dwWidth;
		dwHeight = m_dwHeight;

		pView = &m_mView2D;
	}
	// else ortho or perspective projection
	else
	{
		dwWidth = m_VP[m_nStage].Width;
		dwHeight = m_VP[m_nStage].Height;

		pView = &m_mView3D;

		if (m_Mode == EMD_PERSPECTIVE)
			pProj = &m_mProjP[m_nStage];
		else
			pProj = &m_mProjO[m_nStage];
	}

	// resize to viewportspace [-1,1] -> projection
	vcS.x = (((pt.x*2.0f) / dwWidth) - 1.0f) / m_mProjP[m_nStage]._11;
	vcS.y = -(((pt.y*2.0f) / dwHeight) - 1.0f) / m_mProjP[m_nStage]._22;
	vcS.z = 1.0f;

	// invert view matrix
	mInvView.InverseOf(*((ZFXMatrix*)&m_mView3D._11));

	// ray from screen to worldspace
	(*vcDir).x = (vcS.x * mInvView._11)
		+ (vcS.y * mInvView._21)
		+ (vcS.z * mInvView._31);
	(*vcDir).y = (vcS.x * mInvView._12)
		+ (vcS.y * mInvView._22)
		+ (vcS.z * mInvView._32);
	(*vcDir).z = (vcS.x * mInvView._13)
		+ (vcS.y * mInvView._23)
		+ (vcS.z * mInvView._33);

	// inverse translation.
	(*vcOrig).x = mInvView._41;
	(*vcOrig).y = mInvView._42;
	(*vcOrig).z = mInvView._43;

	// normalize
	(*vcOrig).Normalize();
}

ZFXVector ZFXOpenGL::Transform2Dto2D(UINT nHwnd, float fScale, const POINT* pPt, ZFXAXIS axis)
{
	ZFXVector vcResult(0, 0, 0);
	POINT ptC = { -1, -1 };
	RECT  rect;
	
	if (!m_bWindowed) return vcResult;
	else if (m_hWnd[nHwnd] == NULL) return vcResult;

	
	GetClientRect(m_hWnd[nHwnd], &rect);

	if (!pPt) 
	{
		GetCursorPos(&ptC);
		ScreenToClient(m_hWnd[nHwnd], &ptC);
	}
	else memcpy(&ptC, pPt, sizeof(POINT));

	ptC.x -= long(((float)rect.right) / 2.0f);
	ptC.y -= long(((float)rect.bottom) / 2.0f);

	// use buttom for both as horizontal view angle is not known
	if (axis == Z_AXIS) 
	{
		vcResult.x = ((float)ptC.x) / rect.bottom * fScale;
		vcResult.y = -((float)ptC.y) / rect.bottom * fScale;
		vcResult.z = 0.0f;
	}
	else if (axis == X_AXIS) 
	{
		vcResult.x = 0.0f;
		vcResult.y = -((float)ptC.y) / rect.bottom * fScale;
		vcResult.z = ((float)ptC.x) / rect.bottom * fScale;
	}
	else if (axis == Y_AXIS) 
	{
		vcResult.x = ((float)ptC.x) / rect.bottom * fScale;
		vcResult.y = 0.0f;
		vcResult.z = -((float)ptC.y) / rect.bottom * fScale;
	}
	return vcResult;
}

POINT ZFXOpenGL::Transform3Dto2D(const ZFXVector &vcPoint)
{
	POINT pt;
	float fClip_x, fClip_y;
	float fXp, fYp, fWp;
	DWORD dwWidth, dwHeight;

	// if 2D mode use whole screen
	if (m_Mode == EMD_TWOD) 
	{
		dwWidth = m_dwWidth;
		dwHeight = m_dwHeight;
	}
	// else take viewport dimensions
	else
	{
		dwWidth = m_VP[m_nStage].Width,
			dwHeight = m_VP[m_nStage].Height;
	}

	fClip_x = (float)(dwWidth >> 1);
	fClip_y = (float)(dwHeight >> 1);

	fXp = (m_mViewProj._11*vcPoint.x) + (m_mViewProj._21*vcPoint.y)
		+ (m_mViewProj._31*vcPoint.z) + m_mViewProj._41;
	fYp = (m_mViewProj._12*vcPoint.x) + (m_mViewProj._22*vcPoint.y)
		+ (m_mViewProj._32*vcPoint.z) + m_mViewProj._42;
	fWp = (m_mViewProj._14*vcPoint.x) + (m_mViewProj._24*vcPoint.y)
		+ (m_mViewProj._34*vcPoint.z) + m_mViewProj._44;

	float fWpInv = 1.0f / fWp;

	// transform from [-1,1] to actual viewport dimensions
	pt.x = (LONG)((1.0f + (fXp * fWpInv)) * fClip_x);
	pt.y = (LONG)((1.0f + (fYp * fWpInv)) * fClip_y);

	return pt;
	//throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::SetWorldTransform(const ZFXMatrix* m)
{
	// flush vertex manager 
	throw std::logic_error("Flush vertex cache");

	if (!m)
	{
		m_mWorld3D.Identity();
	}
	else
	{
		m_mWorld3D = *m;
	}
	GLfloat mat[16];
	MakeGLMatrix(mat, m_mView3D * m_mWorld3D);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mat);

	CalcWorldViewProjMatrix();

	if (m_bCanDoShaders)
	{
		// 将mvp 传给 shader
		throw std::logic_error("MVP shader Update");
	}
	//throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::SetBackfaceCulling(ZFXRENDERSTATE rs)
{
	throw std::logic_error("Flush vertex cache");
	GLenum mode;
	if (rs == RS_CULL_CW)
	{
		mode = GL_BACK;
	}
	else if (rs == RS_CULL_CCW)
	{
		mode = GL_FRONT;
	}
	else if (rs == RS_CULL_NONE)
	{
		glDisable(GL_CULL_FACE);
		return;
	}
	glEnable(GL_CULL_FACE);
	glCullFace(mode);
}

void ZFXOpenGL::SetStencilBufferMode(ZFXRENDERSTATE rs, DWORD dw)
{
//	throw std::logic_error("Flush vertex cache");
//	
//	switch (rs) 
//	{
//		// switch on and off
//	case RS_STENCIL_DISABLE:
//		glDisable(GL_STENCIL_TEST);
//		break;
//	case RS_STENCIL_ENABLE:
//		glEnable(GL_STENCIL_TEST);
//		break;
//	case RS_DEPTHBIAS:
//		//m_pDevice->SetRenderState(D3DRS_DEPTHBIAS, dw);
//		break;
//		// function modes and values
//	case RS_STENCIL_FUNC_ALWAYS:
//		m_pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
//		break;
//	case RS_STENCIL_FUNC_LESSEQUAL:
//		m_pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL);
//		break;
//	case RS_STENCIL_MASK:
//		m_pDevice->SetRenderState(D3DRS_STENCILMASK, dw);
//		break;
//	case RS_STENCIL_WRITEMASK:
//		m_pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, dw);
//		break;
//	case RS_STENCIL_REF:
//		m_pDevice->SetRenderState(D3DRS_STENCILREF, dw);
//		break;
//
//		// stencil test fails modes
//	case RS_STENCIL_FAIL_DECR:
//		m_pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_DECR);
//		break;
//	case RS_STENCIL_FAIL_INCR:
//		m_pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_INCR);
//		break;
//	case RS_STENCIL_FAIL_KEEP:
//		m_pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
//		break;
//
//		// stencil test passes but z test fails modes
//	case RS_STENCIL_ZFAIL_DECR:
//		m_pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR);
//		break;
//	case RS_STENCIL_ZFAIL_INCR:
//		m_pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCR);
//		break;
//	case RS_STENCIL_ZFAIL_KEEP:
//		m_pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
//		break;
//
//		// stencil test passes modes
//	case RS_STENCIL_PASS_DECR:
//		m_pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);
//		break;
//	case RS_STENCIL_PASS_INCR:
//		m_pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
//		break;
//	case RS_STENCIL_PASS_KEEP:
//		m_pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
//		break;
//	} // switch
	//throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::UseStencilShadowSettings(bool)
{
	//throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::UseColorBuffer(bool b)
{
	throw std::logic_error("flush vertex manager.");
	throw std::logic_error("invalidate states.");

	m_bColorBuffer = b;
	if (!b)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE);
	}
	else
	{
		glDisable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ZERO);
	}
}

bool ZFXOpenGL::IsUseColorBuffer(void)
{
	return m_bColorBuffer;
}

void ZFXOpenGL::UseTextures(bool b)
{
	if (m_bTextures == b) return;

	throw std::logic_error("flush vertex manager.");
	throw std::logic_error("invalidate states.");

	m_bTextures = b;
}

bool ZFXOpenGL::IsUseTextures(void)
{
	return m_bTextures;
}

void ZFXOpenGL::SetDepthBufferMode(ZFXRENDERSTATE rs)
{
	throw std::logic_error("flush vertex manager.");

	if (rs == RS_DEPTH_READWRITE)
	{
		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}
	else if (rs == RS_DEPTH_READONLY)
	{
		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}
	else if (rs == RS_DEPTH_NONE)
	{
		glDisable(GL_DEPTH_TEST);
	}
}

void ZFXOpenGL::SetShadeMode(ZFXRENDERSTATE, float, const ZFXCOLOR*)
{
	throw std::logic_error("The method or operation is not implemented.");
}

ZFXRENDERSTATE ZFXOpenGL::GetShadeMode(void)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::SetTextureStage(UCHAR n, ZFXRENDERSTATE rs)
{
	if (!(GLEW_VERSION_1_3 ||
		GLEW_ARB_texture_env_combine ||
		GLEW_EXT_texture_env_combine))
	{
		return E_FAIL;
	}

	GLint max_units = 0;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_units);
	if (n >= max_units)
		return E_FAIL;

	switch (rs)
	{
	case RS_NONE:
		if (ActivateGLTextureUnit(n))
		{
			glDisable(GL_TEXTURE_2D);
		}
		break;
	default:
	case RS_TEX_ADDSIGNED:
		m_mapTextureOp[n] = GL_ADD_SIGNED;
		break;
	case RS_TEX_MODULATE:
		m_mapTextureOp[n] = GL_MODULATE;
		break;
	}
}

HRESULT ZFXOpenGL::SetLight(const ZFXLIGHT* pLight, UCHAR nStage)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	GLfloat mat[16];
	MakeGLMatrix(mat, m_mView3D * m_mWorld3D);
	glLoadMatrixf(mat);

	GLenum gl_index = GL_LIGHT0 + nStage;

	if (!pLight)
	{
		glDisable(gl_index);
		return ZFX_OK;
	}

	switch (pLight->Type)
	{
	case LGT_SPOT:
		glLightf(gl_index, GL_SPOT_CUTOFF, RADIAN2DEGREE(pLight->fPhi));
		glLightf(gl_index, GL_SPOT_EXPONENT, 1.0f);
		break;
	default:
		glLightf(gl_index, GL_SPOT_CUTOFF, 180.0f);
		break;
	}

	// 漫反射
	glLightfv(gl_index, GL_DIFFUSE, (GLfloat*)pLight->cDiffuse.c);

	// 镜面反射
	glLightfv(gl_index, GL_SPECULAR, (GLfloat*)pLight->cSpecular.c);

	// 环境光
	glLightfv(gl_index, GL_AMBIENT, (GLfloat*)pLight->cAmbient.c);

	// 位置
	glLightfv(gl_index, GL_POSITION, (GLfloat*)(&(pLight->vcPosition)));

	// 方向
	if (pLight->Type == LGT_SPOT || pLight->Type == LGT_DIRECTIONAL)
	{
		glLightfv(gl_index, GL_SPOT_DIRECTION, (GLfloat*)(&(pLight->vcDirection)));
	}

	glLightf(gl_index, GL_CONSTANT_ATTENUATION, (GLfloat)pLight->fAttenuation0);
	glLightf(gl_index, GL_LINEAR_ATTENUATION, (GLfloat)pLight->fAttenuation1);
	glLightf(gl_index, GL_QUADRATIC_ATTENUATION, 1.0f);

	glEnable(gl_index);
	glEnable(GL_LIGHTING);
	return ZFX_OK;
}

HRESULT ZFXOpenGL::CreateVShader(const void *pData, UINT nSize, bool bLoadFromFile, bool bIsCompiled, UINT *pID)
{
	HRESULT result = 0;
	HANDLE hFile = NULL, hMap = NULL;
	DWORD *pShaderSource = NULL;
	if (m_nVShaderNum >= (MAX_SHADER - 1))
		return ZFX_OUTOFMEMORY;

	// create shader object
	GLuint shader = glCreateShader(GL_VERTEX_SHADER);
	if (shader == 0)
	{
		Log("error create shader");
		return E_FAIL;
	}

	
	GLchar **str = (GLchar**)&pShaderSource;
	glShaderSource(shader, 1, str, NULL);
	
	
	GLuint programID = glCreateProgram();
	glAttachShader(programID, shader);

}

HRESULT ZFXOpenGL::CreatePShader(const void *pData, UINT nSize, bool bLoadFromFile, bool bIsCompiled, UINT *pID)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::ActivateVShader(UINT id, ZFXVERTEXID VertexID)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::ActivatePShader(UINT id)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::UseWindow(UINT nHwnd)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::BeginRendering(bool bClearPixel, bool bClearDepth, bool bClearStencil)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::EndRendering(void)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::Clear(bool, bool, bool)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::SetClearColor(float fRed, float fGreen, float fBlue)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::FadeScreen(float fR, float fG, float fB, float fA)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::CreateFont(const char*, int, bool, bool, bool, DWORD, UINT*)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::DrawText(UINT, int, int, UCHAR, UCHAR, UCHAR, char*, ...)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::DrawText(UINT, int, int, DWORD, char*, ...)
{
	throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::DrawText(UINT, int, int, DWORD, char*)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void ZFXOpenGL::SetAmbientLight(float fRed, float fGreen, float fBlue)
{
	GLfloat lmodel_ambient[] = { fRed, fGreen, fBlue, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	//throw std::logic_error("The method or operation is not implemented.");
}

HRESULT ZFXOpenGL::Init(HWND mainWnd, const HWND* childWnds, int nWndsNum, int nMinDepth, int nMinStencil, bool bSaveLog)
{
	HRESULT result = E_FAIL;

	g_bLF = bSaveLog;

	if (nWndsNum > 0)
	{
		if (nWndsNum > MAX_3DHWND) nWndsNum = MAX_3DHWND;
		memcpy(&m_hWnd[0], childWnds, sizeof(HWND) * nWndsNum);
		m_nNumhWnd = nWndsNum;

		for (int i = 0; i < nWndsNum; i++)
		{
			m_hDC[i] = GetDC(m_hWnd[i]);
			if (!m_hDC[i])
			{
				Log("GetDC error.");
				return E_FAIL;
			}
			if (!InitPixelFormat(i))
			{
				Log("SetPixelFormat error");
				return E_FAIL;
			}
		}
	}
	else
	{
		m_hWnd[0] = mainWnd;
		m_nNumhWnd = 0;
	}

	if (nMinStencil > 0)
		m_bStencil = true;

	// default Bind m_hDC[0] with m_hRC;
	if (!(m_hRC = wglCreateContext(m_hDC[0])))
	{
		Log("CreateContext error");
		return E_FAIL;
	}

	if (!wglMakeCurrent(m_hDC[0], m_hRC))
	{
		Log("Bind Context error");
		return E_FAIL;
	}

	glShadeModel(GL_SMOOTH);

	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);

	glEnable(GL_DEPTH_TEST);

	std::logic_error("enum parameter");

	return Go();
}

HRESULT ZFXOpenGL::Go(void)
{
	
}

HRESULT ZFXOpenGL::InitWindowed(HWND mainWnd, const HWND* childWnds, int nWndsNum, bool bSaveLog)
{

}

void ZFXOpenGL::Release(void)
{

}

void ZFXOpenGL::UseShaders(bool)
{

}

HRESULT ZFXOpenGL::SetShaderConstant(ZFXSHADERTYPE, ZFXDATATYPE, UINT, UINT, const void*)
{

}

void ZFXOpenGL::UseAdditiveBlending(bool)
{

}

HRESULT ZFXOpenGL::SetView3D(const ZFXVector &vcRight,
	const ZFXVector &vcUp,
	const ZFXVector &vcDir,
	const ZFXVector &vcPos)
{
	if (!m_bRunning) return E_FAIL;

	m_mView3D._14 = m_mView3D._21 = m_mView3D._34 = 0.0f;
	m_mView3D._44 = 1.0f;

	m_mView3D._11 = vcRight.x;
	m_mView3D._21 = vcRight.y;
	m_mView3D._31 = vcRight.z;
	m_mView3D._41 = -(vcRight * vcPos);

	m_mView3D._12 = vcUp.x;
	m_mView3D._22 = vcUp.y;
	m_mView3D._32 = vcUp.z;
	m_mView3D._42 = -(vcUp * vcPos);

	m_mView3D._13 = vcDir.x;
	m_mView3D._23 = vcDir.y;
	m_mView3D._33 = vcDir.z;
	m_mView3D._43 = -(vcDir*vcPos);

	if (!m_bUseShaders)
	{
		GLfloat mat[16];
		MakeGLMatrix(mat, m_mView3D * m_mWorld3D);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(mat);
	}

	return ZFX_OK;
}

HRESULT ZFXOpenGL::SetViewLookAt(const ZFXVector&, const ZFXVector&, const ZFXVector&)
{

}

void ZFXOpenGL::MakeGLMatrix(GLfloat gl_matrix[16], ZFXMatrix matrix)
{
	float *p = (float*)&matrix;
	int x = 0;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			gl_matrix[x] = p[j * 4 + i];
			x++;
		}
	}
}

void ZFXOpenGL::Prepare2D(void)
{
	m_mProj2D.Identity();
	m_mView2D.Identity();
	m_mWorld2D.Identity();

	// orthogonal projection matrix
	m_mProj2D._11 = 2.0f / (float)m_dwWidth;
	m_mProj2D._22 = 2.0f / (float)m_dwHeight;
	m_mProj2D._33 = 1.0f / (m_fFar - m_fNear);
	m_mProj2D._43 = -m_fNear*(1.0f / (m_fFar - m_fNear));
	m_mProj2D._44 = 1.0f;

	// 2D view matrix
	float tx, ty, tz;
	tx = -((int)m_dwWidth) + m_dwWidth * 0.5f;
	ty = m_dwHeight - m_dwHeight  * 0.5f;
	tz = m_fNear + 0.1f;

	m_mView2D._22 = -1.0f;
	m_mView2D._41 = tx;
	m_mView2D._42 = ty;
	m_mView2D._43 = tz;
}

void ZFXOpenGL::CalcViewProjMatrix(void)
{
	ZFXMatrix *pA;
	ZFXMatrix *pB;

	// 2D, perspective or orthogonal mode
	if (m_Mode == EMD_TWOD) {
		pA = (ZFXMatrix*)&m_mProj2D;
		pB = (ZFXMatrix*)&m_mView2D;
	}
	else
	{
		pB = (ZFXMatrix*)&m_mView3D;

		if (m_Mode == EMD_PERSPECTIVE)
			pA = (ZFXMatrix*)&(m_mProjP[m_nStage]);
		else
			pA = (ZFXMatrix*)&(m_mProjO[m_nStage]);
	}

	ZFXMatrix *pM = (ZFXMatrix*)&m_mViewProj;
	(*pM) = (*pA) * (*pB);
}

void ZFXOpenGL::CalcWorldViewProjMatrix(void)
{
	ZFXMatrix *pProj;
	ZFXMatrix *pView;
	ZFXMatrix *pWorld;

	pWorld = (ZFXMatrix*)&m_mWorld3D;

	// 2D, perspective or orthogonal mode
	if (m_Mode == EMD_TWOD)
	{
		pProj = (ZFXMatrix*)&m_mProj2D;
		pView = (ZFXMatrix*)&m_mView2D;
	}
	else 
	{
		pView = (ZFXMatrix*)&m_mView3D;

		if (m_Mode == EMD_PERSPECTIVE)
			pProj = (ZFXMatrix*)&(m_mProjP[m_nStage]);
		else
			pProj = (ZFXMatrix*)&(m_mProjO[m_nStage]);
	}

	ZFXMatrix *pCombo = (ZFXMatrix*)&m_mWorldViewProj;
	(*pCombo) = ((*pProj) * (*pView) * (*pWorld));  //((*pWorld) * (*pView)) * (*pProj);
}

HRESULT ZFXOpenGL::CalcPerspProjMatrix(float fFov, float fAspect, ZFXMatrix* m)
{
	if (fabs(m_fFar - m_fNear) < 0.01f)
		return ZFX_FAIL;

	float sinFOV2 = sinf(fFov / 2);

	if (fabs(sinFOV2) < 0.01f)
		return ZFX_FAIL;

	float cosFOV2 = cosf(fFov / 2);

	float w = fAspect * (cosFOV2 / sinFOV2);
	float h = 1.0f  * (cosFOV2 / sinFOV2);
	float Q = m_fFar / (m_fFar - m_fNear);

	memset(m, 0, sizeof(ZFXMatrix));
	(*m)._11 = w;
	(*m)._22 = h;
	(*m)._33 = Q;
	(*m)._34 = 1.0f;
	(*m)._43 = -Q*m_fNear;

	return ZFX_OK;
}

void ZFXOpenGL::CalcOrthoProjMatrix(float l, float r, float b, float t, float fN, float fF, int nStage)
{
	float x = 2.0f / (r - l);
	float y = 2.0f / (t - b);
	float z = 2.0f / (fF - fN);
	float tx = -(r + l) / (r - l);
	float ty = -(t + b) / (t - b);
	float tz = -(fF + fN) / (fF - fN);

	memset(&m_mProjO[nStage], 0, sizeof(ZFXMatrix));
	m_mProjO[nStage]._11 = x;
	m_mProjO[nStage]._22 = y;
	m_mProjO[nStage]._33 = z;
	m_mProjO[nStage]._44 = 1.0f;
	m_mProjO[nStage]._41 = tx;
	m_mProjO[nStage]._42 = ty;
	m_mProjO[nStage]._43 = tz;
}

bool ZFXOpenGL::InitPixelFormat(int nHWnd)
{
	UINT iformat = -1;
	if (m_hDC[nHWnd] == NULL)
		return false;

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 16;

	if (!(iformat = ChoosePixelFormat(m_hDC[nHWnd], &pfd)))
		return false;

	return SetPixelFormat(m_hDC[nHWnd], iformat, &pfd);
}

/**
* write outputstring to attribut outputstream if exists
* -> IN: char - format string to output
*        ...  - output values
*/
void ZFXOpenGL::Log(char *chString, ...)
{
	char ch[1024];
	char *pArgs;

	pArgs = (char*)&chString + sizeof(chString);
	vsprintf(ch, chString, pArgs);
	fprintf(m_pLog, "[%s]: ", GetName().c_str());
	fprintf(m_pLog, ch);
	fprintf(m_pLog, "\n");

	if (g_bLF)
		fflush(m_pLog);
} // Log

std::string ZFXOpenGL::GetName()
{
	static std::string APIName("OpenGL Device");
	return APIName;
}

bool ZFXOpenGL::ActivateGLTextureUnit(UCHAR n)
{
	if (m_nActivateTextureUnit == n)
		return true;

	GLint max_unit = 0;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_unit);
	if (GLEW_VERSION_1_3 && n < max_unit)
	{
		glActiveTexture(GL_TEXTURE0 + max_unit);
		m_nActivateTextureUnit = n;
		return true;
	}
	else
	{
		return false;
	}
		
}


