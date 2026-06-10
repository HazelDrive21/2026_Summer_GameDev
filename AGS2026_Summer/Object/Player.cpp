#include <DxLib.h>
#include <string>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/Camera.h"
#include "../Audio/AudioManager.h"
#include "Common/AnimationController.h"
#include "../Object/Collider/ColliderLine.h"
#include "../Object/Collider/ColliderCapsule.h"
#include "../Object/Collider/ColliderModel.h"
#include "../Object/Enemy/EnemyManager.h"
#include "../Object/Weapon/WeaponBlade.h"
#include "FCS.h"
#include "Player.h"

int Player::s_rightArmEquipID = 0;
int Player::s_rightBackEquipID = 0;
int Player::s_leftArmEquipID = 0;
int Player::s_leftBackEquipID = 0;

Player::Player(void)
{

	animationController_ = nullptr;
	fcs_ = nullptr;
	state_ = STATE::NONE;

	speed_ = 0.0f;
	moveDir_ = AsoUtility::VECTOR_ZERO;
	movePow_ = AsoUtility::VECTOR_ZERO;
	movedPos_ = AsoUtility::VECTOR_ZERO;
	velocity_ = AsoUtility::VECTOR_ZERO;

	playerRotY_ = Quaternion();
	goalQuaRot_ = Quaternion();
	stepRotTime_ = 0.0f;

	jumpPow_ = AsoUtility::VECTOR_ZERO;
	isJump_ = false;
	stepJump_ = 0.0f;

	// 衝突チェック
	gravHitPosDown_ = AsoUtility::VECTOR_ZERO;
	gravHitPosUp_ = AsoUtility::VECTOR_ZERO;

	imgShadow_ = -1;

	currentTurnSpeed_ = DEFAULT_TURN_SPEED;

	airDashTime_ = 0.0f;

	en_ = MAX_EN;

	maxHp_ = 40000;
	hp_ = maxHp_;

	isCharging_ = false;

	//// 例：名前"RIFLE", 総弾数, 発射間隔, 弾速, 威力, 寿命
	//rightWeapon_ = new WeaponFirearm("MACHINE GUN", 800, 5, 100.0f, 50, 600);

	//// 右肩にミサイルを装備 (例: 弾数20発、リロード120F、弾速40.0、威力500、寿命300F)
	//rightBackWeapon_ = new WeaponMissile("VERTICAL MISSILE", 200, 120, 40.0f, 500, 300);

	//// ブレード武器を装備 (例: 名前"LASER BLADE", 威力60、リーチ1500、寿命10F)
	//leftWeapon_ = new WeaponBlade("LASER BLADE", 60, 1500, 10);

	

}

Player::~Player(void)
{

	// 武器の解放
	if (rightWeapon_ != nullptr) { delete rightWeapon_; }
	if (rightBackWeapon_ != nullptr) { delete rightBackWeapon_; }
	if (leftWeapon_ != nullptr) { delete leftWeapon_; }
	if (leftBackWeapon_ != nullptr) { delete leftBackWeapon_; }

	// 残っている弾の解放
	for (auto bullet : activeBullets_) { delete bullet; }
	activeBullets_.clear();

	delete animationController_;
	delete fcs_;
}

void Player::ApplyDamage(int damage)
{
	// すでに死亡している場合は重ねて処理しない
	if (hp_ <= 0) return;

	hp_ -= damage;

	// マイナスにならないようにクランプ
	if (hp_ < 0)
	{
		hp_ = 0;
		// 必要であれば、ここに死亡状態（STATE::DEADなど）への遷移を書く
	}
}

void Player::InitEquippedWeapons(void)
{
	// ─── 既存の武器がある場合はメモリリーク防止のために破棄 ───
	if (rightWeapon_) { delete rightWeapon_;     rightWeapon_ = nullptr; }
	if (rightBackWeapon_) { delete rightBackWeapon_; rightBackWeapon_ = nullptr; }
	if (leftWeapon_) { delete leftWeapon_;      leftWeapon_ = nullptr; }
	if (leftBackWeapon_) { delete leftBackWeapon_;  leftBackWeapon_ = nullptr; }

	// ─── ① 右手武器の生成 (s_rightArmEquipID に応じて分岐) ───
	switch (s_rightArmEquipID)
	{
	case 0: // RIFLE
		//					引数: 名前, 最大弾数, リロードフレーム, 弾速, ダメージ, 射程距離, 弾の半径, 弾の色, FCSタイプ, EN消費量, 重量
		rightWeapon_ = new WeaponFirearm("RIFLE", 120, 20, 300.0f, 120, 3000.0f, 20.0f, GetColor(255, 255, 0),FCS::SITE_TYPE::STANDARD,0,250);
		break;

	case 1: // MACHINE GUN
		rightWeapon_ = new WeaponFirearm("MACHINE GUN", 800, 5, 100.0f, 50, 2000.0f, 20.0f, GetColor(255, 255, 0),FCS::SITE_TYPE::WIDE_SHALLOW,0,120);
		break;

	case 2: // SNIPER RIFLE
		rightWeapon_ = new WeaponFirearm("SNIPER RIFLE", 20, 120, 500.0f, 1000, 9000.0f, 20.0f, GetColor(255, 255, 0),FCS::SITE_TYPE::DEEP_NARROW,0,800);
		break;

	case 3: // EN RIFLE
		rightWeapon_ = new WeaponFirearm("EN RIFLE", 60, 30, 400.0f, 400, 4000.0f, 20.0f, GetColor(5, 12, 255),FCS::SITE_TYPE::LARGE,50,500);
		break;

	case 4: // PULSE GUN
		rightWeapon_ = new WeaponFirearm("PULSE GUN", 300, 5, 200.0f, 160, 2500.0f, 20.0f, GetColor(5, 12, 255),FCS::SITE_TYPE::WIDE_SHALLOW,5,240);
		break;

	case 5: // EN SNIPER RIFLE
		rightWeapon_ = new WeaponFirearm("EN SNIPER RIFLE", 20, 120, 900.0f, 3000, 9000.0f, 20.0f, GetColor(5, 12, 255), FCS::SITE_TYPE::DEEP_NARROW, 1000,1600);
		break;

	case 6: // KARASAWAもどき
		rightWeapon_ = new WeaponFirearm("TAKARADA", 60, 90, 400.0f, 2500, 4000.0f, 20.0f, GetColor(5, 12, 255), FCS::SITE_TYPE::STANDARD, 500, 600, 600.0f, 500);
		break;

	default:
		rightWeapon_ = new WeaponFirearm("RIFLE", 120, 20, 300.0f, 120, 3000.0f, 20.0f, GetColor(255, 255, 0),FCS::SITE_TYPE::STANDARD,0,250);
		break;
	}

	// ─── ② 右肩武器の生成 (s_rightBackEquipID に応じて分岐) ───
	switch (s_rightBackEquipID)
	{
	case 0: // SMALL MISSILE
		rightBackWeapon_ = new WeaponMissile("SMALL MISSILE", 120, 120, 40.0f, 500, 5000.0f, 40.0f, GetColor(255, 60, 60), 6, 1, FCS::SITE_TYPE::WIDE_SHALLOW, 100);
		break;
	case 1: // MULTI MISSILE
		rightBackWeapon_ = new WeaponMissile("MULTI MISSILE", 200, 120, 40.0f, 500, 5000.0f, 40.0f, GetColor(255, 60, 60), 4, 4, FCS::SITE_TYPE::WIDE_SHALLOW, 400);
		break;
	case 2: // CANNON-G7
		rightBackWeapon_ = new WeaponFirearm("CANNON-G7", 20, 120, 150.0f, 1200, 5000.0f, 20.0f, GetColor(255, 100, 0), FCS::SITE_TYPE::DEEP_NARROW, 0, 1400, 1000.0f, 800);
		break;
	case 3: // CANNON-G8
		rightBackWeapon_ = new WeaponFirearm("CANNON-G8", 20, 120, 150.0f, 1800, 5000.0f, 20.0f, GetColor(0, 10, 255), FCS::SITE_TYPE::DEEP_NARROW, 600, 1400, 1000.0f, 800);
		break;
	default:
		rightBackWeapon_ = new WeaponMissile("SMALL MISSILE", 120, 120, 40.0f, 500, 5000.0f, 40.0f, GetColor(255, 60, 60), 6, 1, FCS::SITE_TYPE::WIDE_SHALLOW, 100);
		break;
	}

	// ─── ③ 左手武器の生成 (s_leftArmEquipID に応じて分岐) ───

	// ――― ④ 左肩武器の生成 (s_leftBackEquipID に応じて分岐) ───
	switch (s_leftBackEquipID)
	{
	case 0: // SMALL MISSILE
		leftBackWeapon_ = new WeaponMissile("SMALL MISSILE", 120, 120, 40.0f, 500, 5000.0f, 40.0f, GetColor(255, 60, 60), 6, 1, FCS::SITE_TYPE::WIDE_SHALLOW, 100);
		break;
	case 1: // MULTI MISSILE
		leftBackWeapon_ = new WeaponMissile("MULTI MISSILE", 200, 120, 40.0f, 500, 5000.0f, 40.0f, GetColor(255, 60, 60), 4, 4, FCS::SITE_TYPE::WIDE_SHALLOW, 400);
		break;
	case 2: // CANNON-G7
		leftBackWeapon_ = new WeaponFirearm("CANNON-G7", 20, 120, 150.0f, 1200, 5000.0f, 20.0f, GetColor(255, 100, 0), FCS::SITE_TYPE::DEEP_NARROW, 0, 1400, 1000.0f, 800);
		break;
	case 3: // CANNON-G8
		leftBackWeapon_ = new WeaponFirearm("CANNON-G8", 20, 120, 150.0f, 1800, 5000.0f, 20.0f, GetColor(0, 10, 255), FCS::SITE_TYPE::DEEP_NARROW, 600, 1400, 1000.0f, 800);
		break;
	default:
		leftBackWeapon_ = new WeaponFirearm("CANNON-G7", 20, 120, 150.0f, 1200, 5000.0f, 20.0f, GetColor(255, 100, 0), FCS::SITE_TYPE::DEEP_NARROW, 0, 1400, 1000.0f, 800);
		break;
	}

	// ─── ③ 敵の弾と区別するためのフラグ設定 ───
	if (rightWeapon_)  rightWeapon_->SetEnemyWeapon(false);
	if (rightBackWeapon_) rightBackWeapon_->SetEnemyWeapon(false);
	if (leftWeapon_)   leftWeapon_->SetEnemyWeapon(false);
	if (leftBackWeapon_) leftBackWeapon_->SetEnemyWeapon(false);
}

int Player::CalcTotalWeaponWeight(void) const
{
	int totalWeight = 0;

	// 右手武器の重量を加算
	if (rightWeapon_ != nullptr)
	{
		totalWeight += rightWeapon_->GetWeight();
	}

	// 右肩武器の重量を加算
	if (rightBackWeapon_ != nullptr)
	{
		totalWeight += rightBackWeapon_->GetWeight();
	}

	if (leftWeapon_ != nullptr)      totalWeight += leftWeapon_->GetWeight();
	if (leftBackWeapon_ != nullptr)   totalWeight += leftBackWeapon_->GetWeight();

	return totalWeight;
}

float Player::GetWeightSpeedMultiplier(void) const
{
	int weight = CalcTotalWeaponWeight();

	// 【例】基準重量を 400 とし、それを超えた分だけ速度が低下する（簡易アセンブルシミュレーション）
	// パラメーターはゲームバランスを見て調整してください。
	float baseWeight = 1000.0f;
	if (weight <= baseWeight) return 1.0f; // 基準以下なら速度低下なし

	// 超過重量に応じて減算（例：100重くなるごとに2.5%遅くなる）
	float excess = static_cast<float>(weight) - baseWeight;
	float penalty = (excess / 100.0f) * 0.025f;

	// 最低でも本来の速度の 70% は維持するガード
	float multiplier = 1.0f - penalty;
	if (multiplier < 0.70f) multiplier = 0.70f;

	return multiplier;
}

void Player::InitLoad(void)
{
	transform_.SetModel(
		resMng_.LoadModelDuplicate(ResourceManager::SRC::PLAYER));

	// 丸影画像
	imgShadow_ = resMng_.Load(ResourceManager::SRC::PLAYER_SHADOW).handleId_;
}

void Player::InitTransform(void)
{
	transform_.scl = { 8.0f,8.0f,8.0f };
	transform_.pos = { 0.0f, 5000.0f, 0.0f };
	transform_.quaRot = Quaternion();
	transform_.quaRotLocal = Quaternion::Euler({ 0.0f, AsoUtility::Deg2RadF(180.0f), 0.0f });
	transform_.SetEmissive(GetColorF(0.0f, 0.5f, 1.0f, 1.0f), 1);
	transform_.Update();
}

void Player::InitCollider(void)
{
	// 主に地面との衝突で仕様する線分コライダ
	ColliderLine* colLine = new ColliderLine(
		ColliderBase::TAG::PLAYER, &transform_,
		COL_LINE_START_LOCAL_POS, COL_LINE_END_LOCAL_POS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::LINE), colLine);

	// 主に壁や木などの衝突で仕様するカプセルコライダ
	ColliderCapsule* colCapsule = new ColliderCapsule(
		ColliderBase::TAG::PLAYER, &transform_,
		COL_CAPSULE_TOP_LOCAL_POS, COL_CAPSULE_DOWN_LOCAL_POS,
		COL_CAPSULE_RADIUS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::CAPSULE), colCapsule);
}

void Player::InitAnimation(void)
{

	std::string path = Application::PATH_MODEL + "Player/";
	animationController_ = new AnimationController(transform_.modelId);
	animationController_->Add((int)ANIM_TYPE::IDLE, path + "Idle.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::RUN, path + "Run.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::FAST_RUN, path + "FastRun.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::JUMP, path + "Jump.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::WARP_PAUSE, path + "WarpPose.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::FLY, path + "Flying.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::FALLING, path + "Falling.mv1", 80.0f);
	animationController_->Add((int)ANIM_TYPE::VICTORY, path + "Victory.mv1", 60.0f);

	animationController_->Play((int)ANIM_TYPE::IDLE);

}

void Player::InitPost(void)
{
	speed_ = 0.0f;
	moveDir_ = VGet(0, 0, 1); // 初期方向をZ正(正面)に設定
	movePow_ = AsoUtility::VECTOR_ZERO;
	jumpPow_ = AsoUtility::VECTOR_ZERO;
	dashResidualTimer_ = 0.0f;

	// 高度が空中（例えば Y > 100.0f）なら、最初から空中状態にする
	if (transform_.pos.y > 140.0f)
	{
		isJump_ = true;
		isGrounded_ = false;
	}
	else
	{
		isJump_ = false;
		isGrounded_ = true;
	}

	isBoostAscent_ = false;

	playerRotY_ = transform_.quaRot;
	goalQuaRot_ = transform_.quaRot;

	// ★★★ FCSの生成と初期化 ★★★
	fcs_ = new FCS();
	fcs_->Init();
	fcs_->SetPlayer(this); // FCS側に自分（Player）への参照を渡す

	SetUseLighting(FALSE);

	en_ = MAX_EN;

	InitEquippedWeapons();

	ChangeState(STATE::PLAY);
}

void Player::UpdateProcess(void)
{
	float weightMultiplier = GetWeightSpeedMultiplier();
	float currentTurnSpeed = TURN_SPEED * weightMultiplier;

	// カメラの視点移動速度（マウスや右スティックの追従）にもこの重量感を反映
	auto* camera = SceneManager::GetInstance().GetCamera();
	camera->SetRotationSpeed(currentTurnSpeed);

	// 2. 更新ステップ（ここには純粋な移動や硬直タイマーの処理だけが残る）
	switch (state_)
	{
	case Player::STATE::NONE:
		UpdateNone();
		break;
	case Player::STATE::PLAY:
		UpdatePlay();
		break;
	case Player::STATE::STOP:
		UpdateStop();
		break;
	case Player::STATE::LANDING_STIFF:
		UpdateLandingStiff();
		break;
	}

	// ★ 3. 硬直中であっても、FCS・武器・弾丸は常に更新・発射できるようにする
	if (state_ == STATE::PLAY || state_ == STATE::STOP || state_ == STATE::LANDING_STIFF)
	{
		ProcessTurn();
		UpdateEnergy(scnMng_.GetDeltaTime());
		UpdateCommonMechanics();
		UpdateAntiMissile();
	}

	// --- 左手武器（ブレード）の攻撃処理例 ---
	if (InputManager::GetInstance().IsPadBtnTrgDown(InputManager::JOYPAD_NO::KEY_PAD1, InputManager::JOYPAD_BTN::RIGHT))
	{
		if (leftWeapon_ != nullptr && leftWeapon_->IsReady())
		{
			auto& bulletList = SceneManager::GetInstance().GetBulletList();

			// 1. 前方ベクトルの計算
			VECTOR forward = VTransform(VGet(0.0f, 0.0f, 1.0f), transform_.matRot);

			// 2. 突進速度の反映
			velocity_ = VScale(forward, 25.0f);

			// 3. 判定の生成
			VECTOR muzzlePos = VAdd(transform_.pos, VScale(forward, 30.0f));
			leftWeapon_->Fire(muzzlePos, AsoUtility::VECTOR_ZERO, bulletList, false);
		}
	}

	// 武器自体のリロード更新
	if (leftWeapon_ != nullptr) { leftWeapon_->Update(); }
}

void Player::UpdateProcessPost(void)
{
}

void Player::Draw(void)
{

	CharactorBase::Draw();

#ifdef _DEBUG
	//DrawFormatString(0, 180, GetColor(255, 255, 255),
		//"Pos: X=%.1f Y=%.1f Z=%.1f",
		//transform_.pos.x, transform_.pos.y, transform_.pos.z);
	DrawFormatString(0, 200, GetColor(255, 255, 255), "jumpPow.y: %.2f", jumpPow_.y);
	DrawFormatString(0, 220, GetColor(255, 255, 255), "MoveSpeed: %.2f", debugCurrentSpeed_);
	//DrawFormatString(0, 240, GetColor(255, 255, 255), "gravityScale: %.1f", gravityScale_);
	//DrawFormatString(0, 260, GetColor(255, 255, 255), "isGrounded: %d", isGrounded_ ? 1 : 0);
	//DrawFormatString(0, 280, GetColor(255, 255, 255), "EN: %.1f / %.1f", en_, MAX_EN);
#endif // _DEBUG

	// 迎撃演出の描画処理
	for (auto it = antiMissileEffects_.begin(); it != antiMissileEffects_.end(); )
	{
		// 線を描画
		DrawLine3D(it->start, it->end, GetColor(0, 255, 128));

		// 寿命カウントダウン
		it->life--;
		if (it->life <= 0) {
			it = antiMissileEffects_.erase(it);
		}
		else {
			++it;
		}
	}

	// モデルの描画
	MV1DrawModel(transform_.modelId);

	// 丸影描画
	DrawShadow();

	for (auto bullet : activeBullets_)
	{
		if (bullet != nullptr)
		{
			bullet->Draw();
		}
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128); // 128/255 の不透明度で半透明化

	for (const auto& eff : explosionEffects_)
	{
		// オレンジ〜赤色の半透明な球体を描画して爆風を表現
		DrawSphere3D(eff.pos, eff.currentRadius, 8, GetColor(255, 128, 0), GetColor(255, 0, 0), FALSE);
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // 描画が終わったらブレンドモードを戻す

	// 画面左下にHP（AP）をメーターと数値で表示する例
	int barX = 50;
	int barY = Application::SCREEN_SIZE_Y - 80;
	int barWidth = 300;
	int barHeight = 15;

	// ① HPバーの背景（黒い枠）
	DrawBox(barX, barY, barX + barWidth, barY + barHeight, GetColor(50, 50, 50), TRUE);

	// ② 現在のHPの割合を計算
	float hpRate = static_cast<float>(hp_) / maxHp_;
	int currentBarWidth = static_cast<int>(barWidth * hpRate);

	// ③ HPバーの本体（AC2AA風なら緑や青緑系、ピンチで赤にするなど）
	unsigned int barColor = GetColor(0, 255, 200); // 通常時はシアン（青緑）
	if (hpRate < 0.3f)
	{
		barColor = GetColor(255, 0, 0); // 残り3割以下で赤ゲージに
	}
	DrawBox(barX, barY, barX + currentBarWidth, barY + barHeight, barColor, TRUE);

	// ④ 文字で数値を表示
	// AC風に "AP  8000" のような表記にすると気分が上がります！
	DrawFormatString(barX, barY - 25, GetColor(255, 255, 255), "AP %4d / %4d", hp_, maxHp_);

	//DrawFormatString(0, 300, GetColor(255, 0, 0), "isCharging: %d", isCharging_ ? 1 : 0);

	/*DrawFormatString(0, 80, GetColor(255, 255, 255), "Timer: %f", dashResidualTimer_);
	DrawFormatString(0, 100, GetColor(255, 255, 255), "isJump: %d", isJump_ ? 1 : 0);
	DrawFormatString(0, 120, GetColor(255, 255, 255), "Speed: %f", speed_);
	DrawFormatString(0, 140, GetColor(255, 255, 255), "DashDuration: %f", dashPressDuration_);
	DrawFormatString(0, 160, GetColor(255, 255, 255), "isDashPress: %d", isDashKeyPress_ ? 1 : 0);
	DrawFormatString(0, 180, GetColor(255, 255, 255), "isBoostAscent: %d", isBoostAscent_ ? 1 : 0);*/



}

void Player::ChangeState(STATE state)
{

	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	switch (state_)
	{
	case Player::STATE::NONE:
		ChangeStateNone();
		break;
	case Player::STATE::PLAY:
		ChangeStatePlay();
		break;
	case Player::STATE::STOP:
		ChangeStateStop();
		break;
	case Player::STATE::LANDING_STIFF:
		AudioManager::GetInstance()->PlaySE(SoundID::SE_MOVE);
		landingStiffTimer_ = TIME_LANDING_STIFF;
		speed_ = 0.0f;
		movePow_ = AsoUtility::VECTOR_ZERO;
		// ズドンと膝をつくような着地モーション（既存のJUMP等の後半フレームを固定しても良いです）
		if (animationController_ != nullptr) {
			animationController_->Play((int)ANIM_TYPE::JUMP, false, 35.0f, 45.0f, false, false);
		}
		break;
	}

}

void Player::ChangeStateNone(void)
{
}

void Player::ChangeStatePlay(void)
{
}

void Player::ChangeStateStop(void)
{
	state_ = STATE::STOP;
	stopTimer_ = STOP_TIME;
	speed_ = 0.0f;
	movePow_ = AsoUtility::VECTOR_ZERO;

	// 急停止した瞬間のカメラの正面を向く（Yを0にして正規化）
	Camera* cam = SceneManager::GetInstance().GetCamera();
	VECTOR camForward = cam->GetForward();
	camForward.y = 0.0f;
	if (VSize(camForward) > 0.001f) {
		camForward = VNorm(camForward);
		goalQuaRot_ = Quaternion::LookRotation(camForward);
	}

	animationController_->Play((int)ANIM_TYPE::IDLE);
}

void Player::UpdateNone(void)
{
}

void Player::UpdatePlay(void)
{
	float deltaTime = scnMng_.GetDeltaTime();

	// --- ダッシュボタンのダブルタップ & 長押し解析システム ---
	InputManager& input = InputManager::GetInstance();
	bool isBoostKeyTrg = input.IsActionTrgDown(InputManager::ACTION::BOOST);
	bool isBoostKeyPress = input.IsActionPush(InputManager::ACTION::BOOST);

	// ダブルタップタイマーの更新
	if (dashButtonTapTimer_ > 0.0f) {
		dashButtonTapTimer_ -= deltaTime;
		if (dashButtonTapTimer_ <= 0.0f) {
			dashButtonTapCount_ = 0; // 猶予時間切れ
		}
	}

	if (isBoostKeyTrg) {
		dashButtonTapCount_++;
		if (dashButtonTapCount_ == 1) {
			dashButtonTapTimer_ = DOUBLE_TAP_LIMIT_TIME;
		}
	}

	// 長押しタイマーの更新
	if (isBoostKeyPress) {
		dashPressDuration_ += deltaTime;
	}
	else {
		dashPressDuration_ = 0.0f;
	}

	// 移動入力・ダッシュ状態の処理
	ProcessMove();
	// ブースト・ジャンプ・上昇の処理
	ProcessJump();

	if (state_ != STATE::PLAY) return;

	// --- アニメーション制御 ---
	if (!isGrounded_)
	{
		if (isBoostAscent_) {
			animationController_->Play((int)ANIM_TYPE::FLY); // 長押し上昇中
		}
		else if (jumpPow_.y < 0.0f) {
			animationController_->Play((int)ANIM_TYPE::FALLING); // 自由落下
		}
	}
	else
	{
		if (boostMode_ == BOOST_MODE::DASH) {
			animationController_->Play((int)ANIM_TYPE::FAST_RUN); // ダッシュ移動
		}
		else if (VSize(movePow_) > 0.1f) {
			animationController_->Play((int)ANIM_TYPE::RUN); // 通常移動
		}
		else {
			animationController_->Play((int)ANIM_TYPE::IDLE);
		}
	}

}

void Player::ProcessMove(void)
{
	InputManager& input = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();

	// --- 1. スティック入力・スライド移動取得 ---
	float stickY = 0.0f;
	XINPUT_STATE xinput;
	if (GetJoypadXInputState(DX_INPUT_PAD1, &xinput) == ERROR_SUCCESS) {
		if (abs(xinput.ThumbLY) > 7849)  stickY = (float)xinput.ThumbLY / 32767.0f;
	}
	bool isL1 = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::L1);
	bool isR1 = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::R1);

	// 移動方向ベクトルの合成用バッファ
	VECTOR localCombinedMoveDir = AsoUtility::VECTOR_ZERO;

	// 【前後移動の合成】
	// ① まずはキーボードのスタック移動（W / S）を反映
	if (input.GetVerticalDir() == InputManager::MoveDir::Up) {
		localCombinedMoveDir.z = 1.0f;
	}
	else if (input.GetVerticalDir() == InputManager::MoveDir::Down) {
		localCombinedMoveDir.z = -1.0f;
	}
	// ② コントローラーの左スティック入力があれば、アナログ操作を活かすために上書き
	if (abs(stickY) > 0.1f) {
		localCombinedMoveDir.z = stickY;
	}

	// 【左右平行移動の合成】
	// ① まずはキーボードのスタック移動（A / D）を反映
	if (input.GetHorizontalDir() == InputManager::MoveDir::Left) {
		localCombinedMoveDir.x = -1.0f;
	}
	else if (input.GetHorizontalDir() == InputManager::MoveDir::Right) {
		localCombinedMoveDir.x = 1.0f;
	}
	// ② コントローラーの L1 / R1 入力があれば、そちらを適用（同時押しはL1優先）
	if (isL1) {
		localCombinedMoveDir.x = -1.0f;
	}
	else if (isR1) {
		localCombinedMoveDir.x = 1.0f;
	}

	// --- ベクトルの計算や慣性・速度処理は元の素晴らしいロジックをそのまま維持 ---
	bool isMovingInput = (VSize(localCombinedMoveDir) > 0.1f);
	if (isMovingInput) {
		float sinY = sinf(transform_.rot.y); float cosY = cosf(transform_.rot.y);
		moveDir_.x = localCombinedMoveDir.x * cosY + localCombinedMoveDir.z * sinY;
		moveDir_.y = 0.0f;
		moveDir_.z = -localCombinedMoveDir.x * sinY + localCombinedMoveDir.z * cosY;
		moveDir_ = AsoUtility::VNormalize(moveDir_);
	}
	else {
		moveDir_ = AsoUtility::VECTOR_ZERO;
	}

	BOOST_MODE prevBoostMode = boostMode_;

	// --- 2. ダッシュ（ブースト移動）状態の判定 ---
	// パッドのDOWNボタン、またはキーボードのSPACEキーのどちらでもダッシュできるように統合
	bool isDashKeyPress = input.IsActionPush(InputManager::ACTION::BOOST);

	if (isGrounded_) {
		// 🔥 ENが0より大きい場合のみダッシュを開始できる
		if (isMovingInput && isDashKeyPress && !isCharging_ && en_ > 0.0f) {
			boostMode_ = BOOST_MODE::DASH;
			dashResidualTimer_ = TIME_DASH_RESIDUAL;
		}
		else if (boostMode_ == BOOST_MODE::DASH) {
			dashResidualTimer_ -= deltaTime;
			if (dashResidualTimer_ <= 0.0f) {
				ChangeState(STATE::STOP);
				boostMode_ = BOOST_MODE::BRAKE;
				return;
			}
		}
	}
	else {
		// 空中時
		if (isMovingInput) {
			if (boostMode_ == BOOST_MODE::DASH || isDashKeyPress) {
				// 🔥 ENが残っている場合のみダッシュを維持・開始
				if (!isCharging_ && en_ > 0.0f) {
					boostMode_ = BOOST_MODE::DASH;
					if (isDashKeyPress) {
						airDashTime_ += deltaTime;
					}
					else {
						airDashTime_ = 0.0f;
					}
				}
				else {
					boostMode_ = BOOST_MODE::NORMAL;
					airDashTime_ = 0.0f;
				}
			}
			else {
				boostMode_ = BOOST_MODE::NORMAL;
				airDashTime_ = 0.0f;
			}
		}
		else {
			boostMode_ = BOOST_MODE::NORMAL;
			airDashTime_ = 0.0f;
		}
	}

	// --- 3. 加減速慣性計算 ---
	float currentAccel = isGrounded_ ? ACCEL_GROUND : ACCEL_AIR;
	if (!isGrounded_) {
		currentAccel = ACCEL_GROUND;

		// 空中切り返し慣性ロジック
		VECTOR flatVelocity = movePow_;
		flatVelocity.y = 0.0f;
		if (VSize(flatVelocity) > 5.0f && VSize(moveDir_) > 0.1f) {
			VECTOR curVelDir = AsoUtility::VNormalize(flatVelocity);
			float dot = curVelDir.x * moveDir_.x + curVelDir.z * moveDir_.z;

			if (dot < 0.5f) {
				float rate = (dot - (-1.0f)) / (0.5f - (-1.0f));
				if (rate < 0.0f) rate = 0.0f;

				float turnInertia = MIN_TURN_ACCEL + (1.0f - MIN_TURN_ACCEL) * rate;
				currentAccel *= turnInertia;
			}
		}
	}

	float currentFriction = isGrounded_ ? FRICTION_GROUND : FRICTION_AIR;

	// 最高速度（基準値）の決定
	float maxSpeed = SPEED_RUN;
	if (boostMode_ == BOOST_MODE::DASH) {
		if (isGrounded_) {
			maxSpeed = SPEED_DASH;
		}
		else {
			// 空中ダッシュの可変加速処理
			constexpr float ACCEL_TIME = 0.6f;

			float ratio = airDashTime_ / ACCEL_TIME;
			if (ratio > 1.0f) ratio = 1.0f;

			float weightMultiplier = GetWeightSpeedMultiplier();
			// ★ 重量ペナルティを適用したベースダッシュ速度を算出
			float dynamicDashSpeed = SPEED_DASH * weightMultiplier;

			float startSpeed = dynamicDashSpeed * 0.8f;
			float endSpeed = dynamicDashSpeed;
			maxSpeed = startSpeed + (endSpeed - startSpeed) * ratio;
		}
	}
	else {
		maxSpeed = SPEED_RUN;
	}

	float weightMultiplier = GetWeightSpeedMultiplier();

	// ① 最高速度に重量ペナルティを掛ける
	// 通常移動、地上ダッシュ、空中ダッシュ(可変加速後)のすべてに均等に適用されます
	maxSpeed *= weightMultiplier;

	// ②【こだわり】重い機体ほど加速が鈍くなるようにする
	// 加速度（currentAccel）にも倍率を掛けることで、最高速度に達するまでの「もっさり感」が表現できます
	currentAccel *= weightMultiplier;

	// ③（おまけ）もし「重い機体は滑りやすく（止まりにくく）したい」場合は、
	// 摩擦係数を少し下げることで、ブレーキ時にズルッと滑る重量感を演出できます
	// currentFriction *= weightMultiplier;

	if (VSize(moveDir_) > 0.1f) {
		float inputLength = VSize(localCombinedMoveDir);
		if (inputLength > 1.0f) inputLength = 1.0f;

		VECTOR targetVelocity = VScale(moveDir_, maxSpeed * inputLength);
		movePow_.x += (targetVelocity.x - movePow_.x) * currentAccel * deltaTime * 10.0f;
		movePow_.z += (targetVelocity.z - movePow_.z) * currentAccel * deltaTime * 10.0f;
	}
	else {
		movePow_.x *= (1.0f - (1.0f - currentFriction) * deltaTime * 60.0f);
		movePow_.z *= (1.0f - (1.0f - currentFriction) * deltaTime * 60.0f);
	}
	//debugCurrentSpeed_ = VSize(movePow_);

	UpdateMovementSound(deltaTime);
}

void Player::ProcessJump(void)
{
	InputManager& input = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();

	// ★ボタンが押された瞬間、および長押し判定をキーボード(SPACE)とパッド(DOWN)で共通化
	bool isBoostKeyTrg = input.IsActionTrgDown(InputManager::ACTION::BOOST);
	bool isBoostKeyPress = input.IsActionPush(InputManager::ACTION::BOOST);

	// ダブルタップが成立したか？
	bool isDoubleTap = (isBoostKeyTrg && dashButtonTapCount_ == 2 && dashButtonTapTimer_ > 0.0f && !isCharging_);

	if (isGrounded_)

	{
		isBoostAscent_ = false;
		gravityScale_ = 1.0f;

		if (isDoubleTap)
		{
			isJump_ = true;
			isGrounded_ = false;
			dashButtonTapCount_ = 0; // カウントリセット

			if (boostMode_ == BOOST_MODE::DASH) {
				// ① ダッシュ中に素早く2回：前方の勢いを保ったままジャンプ！
				jumpPow_.y = POW_JUMP * 0.5f;
			}
			else {
				// ② 静止時（通常時）に2回：少しジャンプ力の高い大ジャンプ！
				jumpPow_.y = POW_JUMP * 1.5f; // 大ジャンプ
			}
			boostMode_ = BOOST_MODE::NORMAL;
			return;
		}

		// ③ 静止時にダッシュボタン長押し（0.2秒以上）で上昇に移行
		if (boostMode_ != BOOST_MODE::DASH && isBoostKeyPress && dashPressDuration_ > 0.2f && !isCharging_ && en_ > 0.0f)
		{
			isJump_ = true;
			isGrounded_ = false;
			isBoostAscent_ = true;
			jumpPow_.y = 5.0f;
		}
	}
	else
	{
		// --- 空中にいるとき ---
		// ④ 空中での長押し上昇判定
		if (isBoostKeyPress && !isCharging_ && en_ > 0.0f)
		{
			isBoostAscent_ = true;
			gravityScale_ = 0.0f;

			jumpPow_.y += BOOSTER_POW * deltaTime * 60.0f;
			if (jumpPow_.y > MAX_ASCENT_SPEED) {
				jumpPow_.y = MAX_ASCENT_SPEED;
			}
		}
		else
		{
			// ボタンを離した、またはENが切れた時は通常の自由落下
			isBoostAscent_ = false;
			gravityScale_ = 2.2f;

			float gravity = 1.0f * gravityScale_ * deltaTime * 60.0f;
			jumpPow_.y -= gravity;

			if (jumpPow_.y < -50.0f) {
				jumpPow_.y = -50.0f;
			}
		}
	}
}

void Player::SetGoalRotate(double rotRad)
{
	// カメラの向きを基準に、入力された移動方向への目標クォータニオンを計算
	VECTOR cameraRot = SceneManager::GetInstance().GetCamera()->GetAngles();
	Quaternion axis = Quaternion::AngleAxis((double)cameraRot.y + rotRad, AsoUtility::AXIS_Y);

	// 目標の向きを更新するだけでOK！
	// あとは ProcessTurn() が毎フレームこの目標に向かって勝手に回ってくれます
	goalQuaRot_ = axis;
}

void Player::Rotate(void)
{
	// 目標回転（旋回によって変化したクォータニオン）を同期
	playerRotY_ = goalQuaRot_;
	transform_.quaRot = playerRotY_;
}

void Player::Collision(void)
{
	// 1. 水平移動量(movePow_) と 垂直移動量(jumpPow_) を現在の座標に直接加算する
	VECTOR prevPos = transform_.pos; // 🔥【追加】移動前の座標を記録しておく

	transform_.pos = VAdd(transform_.pos, movePow_);
	transform_.pos = VAdd(transform_.pos, jumpPow_);

	// 移動した座標をコライダに反映させるため、一度行列を更新
	transform_.Update();

	// 2. 壁や障害物との衝突判定（カプセルによる横押し戻し）
	// これにより、移動して壁にめり込んだ transform_.pos が正しくその場で押し戻されます
	CollisionCapsule();

	// 3. 床（ステージポリゴン）との接地判定
	// 上昇推進力が強くかかっている間、またはブースター上昇フラグが立っている間は、
	// 床に吸い付くのを防ぐために接地チェックをスキップして空中状態にする
	if (jumpPow_.y > 0.05f || isBoostAscent_)
	{
		isGrounded_ = false;
	}
	else
	{
		// 下向きのレイ（線分）を飛ばして床をチェック
		// 壁の押し戻しが完了した「現在の最新の transform_.pos」を基準にする
		VECTOR dirGravity = AsoUtility::DIR_D;
		VECTOR dirUpGravity = AsoUtility::DIR_U;
		float checkPow = 50.0f;

		gravHitPosUp_ = VAdd(transform_.pos, VScale(dirUpGravity, 20.0f));
		gravHitPosDown_ = VAdd(transform_.pos, VScale(dirGravity, checkPow));

		bool isHitFloor = false;
		float highestFloorY = -999999.0f;

		// 登録されているステージコライダをすべてループ
		for (const auto c : hitColliders_)
		{
			if (c->GetShape() == ColliderBase::SHAPE::MODEL)
			{
				auto modelCollider = dynamic_cast<const ColliderModel*>(c);
				if (modelCollider != nullptr)
				{
					int modelId = modelCollider->GetFollow()->modelId;
					// DxLibのポリゴン線分交差判定
					auto hit = MV1CollCheck_Line(modelId, -1, gravHitPosUp_, gravHitPosDown_);

					if (hit.HitFlag > 0)
					{
						if (hit.HitPosition.y > highestFloorY)
						{
							highestFloorY = hit.HitPosition.y;
							isHitFloor = true;
						}
					}
				}
			}
		}

		if (isHitFloor)
		{
			// 接地したため、Y座標を床の高さに合わせる
			transform_.pos.y = highestFloorY + 0.0f;

			// 着地した瞬間の落下速度（引力）を記録しておく
			float landSpeed = jumpPow_.y;
			jumpPow_.y = 0.0f;

			if (isJump_ || !isGrounded_)
			{
				// 高所着地硬直の判定
				if (landSpeed < LIMIT_LANDING_SPEED)
				{
					ChangeState(STATE::LANDING_STIFF);
					boostMode_ = BOOST_MODE::NORMAL;
				}
				else
				{
					// 通常の着地アニメーション
					if (animationController_ != nullptr) {
						animationController_->Play((int)ANIM_TYPE::JUMP, false, 29.0f, 45.0f, false, true);
					}
				}
			}
			isJump_ = false;
			isGrounded_ = true;
		}
		else
		{
			// 下に床がなければ空中（落下状態）
			isGrounded_ = false;
		}
	}

	// 全ての押し戻しと接地処理が完了した最終的な座標を3Dモデル側に即時同期
	transform_.Update();

	VECTOR actualMove = VSub(transform_.pos, prevPos);
	actualMove.y = 0.0f; // 上下移動（落下や上昇）は速度から除外し、純粋な巡航速度にする
	debugCurrentSpeed_ = VSize(actualMove);

	// 4. 入力によって毎フレーム生成される水平移動量だけをリセット
	movePow_ = AsoUtility::VECTOR_ZERO;
}

void Player::CollisionGravity(void)
{
	// 処理はすべて上の Collision() に統合したため、ここは空っぽ（または親を呼ばない形）にします
}


void Player::UpdateMovementSound(float deltaTime)
{
	// 🔥 【対策】今まさにブーストボタンが押されている物理的な入力を取得する
	// （※プロジェクトのInputManagerの仕様、または KEY_INPUT_SPACE などのキー指定に合わせて書き換えてください）
	bool isBoostKeyPressed = InputManager::GetInstance().IsActionPush(InputManager::ACTION::BOOST);
	bool isAscending = (!isGrounded_ && velocity_.y > 0.1f);


	// ─── A. ブースト（ダッシュ・垂直上昇）中の巡航音 ───
	// 「ボタンが押されていて、かつ（ダッシュ中 または 垂直上昇中）」なら音を鳴らす！
	if (isBoostKeyPressed && (boostMode_ == BOOST_MODE::DASH || isAscending))
	{
		footstepTimer_ = 0.0f; // ブースト中は歩行音タイマーをリセット

		// 毎フレーム呼ばれても安全なループ再生
		AudioManager::GetInstance()->PlaySELoop(SoundID::SE_BOOSTING);

		return; // ブースト中は歩行足音の処理をスキップ
	}
	else
	{
		// ボタンを離した、あるいはENが切れて上昇もダッシュも止まったら即座に止める
		AudioManager::GetInstance()->StopSE(SoundID::SE_BOOSTING);
	}


	// ─── C. 通常走行（歩行）の足音 ───
	// 地上にいて、一定以上の移動速度（debugCurrentSpeed_）が出ているとき
	if (isGrounded_ && debugCurrentSpeed_ > 1.0f)
	{
		footstepTimer_ += deltaTime;

		// 例：0.4秒に1回「ガシャン！」と鳴らす
		if (footstepTimer_ >= 0.4f)
		{
			AudioManager::GetInstance()->PlaySE(SoundID::SE_MOVE); // 金属足音
			footstepTimer_ = 0.0f;
		}
	}
	else
	{
		footstepTimer_ = 0.0f; // 停止したらタイマーリセット
	}
}

bool Player::IsEndLanding(void)
{

	bool ret = true;

	// アニメーションがジャンプではない
	if (animationController_->GetPlayType() != (int)ANIM_TYPE::JUMP)
	{
		return ret;
	}

	// アニメーションが終了しているか
	if (animationController_->IsEnd())
	{
		return ret;
	}

	return false;

}

void Player::ProcessTurn(void)
{
	InputManager& input = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();

	// --- 1. 旋回入力の取得 ---
	float turnInput = 0.0f;
	XINPUT_STATE xinput;
	if (GetJoypadXInputState(DX_INPUT_PAD1, &xinput) == ERROR_SUCCESS) {
		if (abs(xinput.ThumbLX) > 7849) {
			turnInput = (float)xinput.ThumbLX / 32767.0f;
		}
	}

	if (input.IsActionPush(InputManager::ACTION::TURN_LEFT))  turnInput = -1.0f;
	if (input.IsActionPush(InputManager::ACTION::TURN_RIGHT)) turnInput = 1.0f;

	// ==========================================================
	// ⚡【追加】武器の総重量による旋回速度ペナルティの適用
	// ==========================================================
	float weightMultiplier = GetWeightSpeedMultiplier();
	float currentTurnSpeed = TURN_SPEED * weightMultiplier;
	// ==========================================================

	// --- 2. 旋回処理の分岐 ---
	if (abs(turnInput) > 0.1f)
	{
		// 【手動旋回】定数 TURN_SPEED の代わりに、重量補正された currentTurnSpeed を使用
		float turnAmountDeg = turnInput * currentTurnSpeed * deltaTime;
		float turnAmountRad = AsoUtility::Deg2RadF(turnAmountDeg);
		Quaternion deltaRot = Quaternion::AngleAxis(turnAmountRad, AsoUtility::AXIS_Y);

		// 現在の回転に直接乗算
		transform_.quaRot = transform_.quaRot.Mult(deltaRot);

		// 手動旋回中は、目標の向き（goalQuaRot_）も現在地に完全同期させておく
		goalQuaRot_ = transform_.quaRot;
	}
	else
	{
		// 【自動旋回】急停止時の補間速度（maxDelta）にも currentTurnSpeed を適用
		Quaternion currentRot = transform_.quaRot;
		float maxDelta = currentTurnSpeed * deltaTime;

		transform_.quaRot = Quaternion::RotateTowards(currentRot, goalQuaRot_, maxDelta);
	}

	// トランスフォームのオイラー角を同期
	transform_.rot = Quaternion::ToEuler(transform_.quaRot);
}

void Player::UpdateCommonMechanics(void)
{
	auto& ins = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();

	// ==========================================================
	// 1. 現在の武器に応じたFCSパラメータ（最大ロック数・射程）の計算と更新
	// ==========================================================
	if (fcs_ != nullptr)
	{
		WeaponBase* activeWp = GetActiveWeapon();
		if (activeWp != nullptr)
		{
			// 💡【解決】ここで「weaponMaxLock」を確実に定義します
			int weaponMaxLock = 1;

			// 🚀 アクティブ武器がミサイルクラスなら、そのミサイルの最大ロック数を取得
			auto* missileWp = dynamic_cast<WeaponMissile*>(activeWp);
			if (missileWp != nullptr)
			{
				weaponMaxLock = missileWp->GetMaxLockCount();
			}

			// 🔍 EnemyManagerが生成されている場合のみFCSをアップデート
			if (EnemyManager::GetInstance() != nullptr)
			{
				fcs_->Update(
					transform_.pos,                                     // 1. myPos
					EnemyManager::GetInstance()->GetEemies(),           // 2. enemies (※実際の定義名 GetEemies に合わせました)
					weaponMaxLock,                                      // 3. weaponMaxLockCount (💡上で定義した変数を渡す)
					activeWp->GetRange(),                               // 4. weaponRange 
					activeWp->GetSiteType()                             // 5. weaponSiteType
				);
			}
		}
	}

	// ==========================================================
	// 2. 武器切り替え処理（⚡3スロットローテーションに拡張）
	// ==========================================================
	bool isSwitchPressed = ins.IsActionTrgDown(InputManager::ACTION::WEAPON_CHANGE);

	if (isSwitchPressed)
	{
		AudioManager::GetInstance()->PlaySE(SoundID::SE_WEAPON_CHANGE);

		// 💡 武器を装備していないスロットは自動でスキップする親切設計
		if (activeWeaponSlot_ == EquipSlot::R_ARM)
		{
			if (rightBackWeapon_ != nullptr)      activeWeaponSlot_ = EquipSlot::R_BACK;
			else if (leftBackWeapon_ != nullptr)  activeWeaponSlot_ = EquipSlot::L_BACK;
		}
		else if (activeWeaponSlot_ == EquipSlot::R_BACK)
		{
			if (leftBackWeapon_ != nullptr)      activeWeaponSlot_ = EquipSlot::L_BACK;
			else                                 activeWeaponSlot_ = EquipSlot::R_ARM;
		}
		else if (activeWeaponSlot_ == EquipSlot::L_BACK)
		{
			activeWeaponSlot_ = EquipSlot::R_ARM;
		}
	}


	// ==========================================================
	// 3. 全武器のタイマー更新 ＆ ミサイルの撃ち終わり（ロック解除）検知
	// ==========================================================
	// ⚡ 右肩と左肩、双方のミサイルコンポーネントを取得
	auto* rMissileWp = dynamic_cast<WeaponMissile*>(rightBackWeapon_);
	auto* lMissileWp = dynamic_cast<WeaponMissile*>(leftBackWeapon_);

	bool wasRLaunching = (rMissileWp != nullptr) ? rMissileWp->IsLaunching() : false;
	bool wasLLaunching = (lMissileWp != nullptr) ? lMissileWp->IsLaunching() : false;

	if (rightWeapon_ != nullptr)     rightWeapon_->Update();
	if (rightBackWeapon_ != nullptr) rightBackWeapon_->Update();
	if (leftBackWeapon_ != nullptr)  leftBackWeapon_->Update();

	// ⚡ どちらかのミサイルが撃ち終わったらロックオンをクリアする
	bool rMissileFinished = (rMissileWp != nullptr && wasRLaunching && !rMissileWp->IsLaunching());
	bool lMissileFinished = (lMissileWp != nullptr && wasLLaunching && !lMissileWp->IsLaunching());

	if ((rMissileFinished || lMissileFinished) && fcs_ != nullptr)
	{
		fcs_->ClearTargets();
	}


	// ==========================================================
	// 4. 共通の攻撃ボタンによる発射処理
	// ==========================================================
	bool isFirePressed = ins.IsActionPush(InputManager::ACTION::FIRE_RIGHT);
	bool isFireTrigger = ins.IsActionPush(InputManager::ACTION::FIRE_RIGHT);

	WeaponBase* activeWp = GetActiveWeapon();

	if (activeWp != nullptr && activeWp->IsReady())
	{
		int requiredEN = activeWp->GetConsumeEN();
		bool isENShortage = (requiredEN > 0) && (isCharging_ || en_ < static_cast<float>(requiredEN));
		bool canFire = !isENShortage;

		if (activeWeaponSlot_ == EquipSlot::R_ARM) // ─── 右腕武器 ───
		{
			if (isFirePressed)
			{
				if (canFire)
				{
					VECTOR localMuzzlePos = VGet(50.0f, 120.0f, 80.0f);
					VECTOR muzzlePos = VAdd(transform_.pos, transform_.quaRot.PosAxis(localMuzzlePos));
					VECTOR targetPos;

					if (fcs_ != nullptr && fcs_->GetLockState() == FCS::LOCK_STATE::LOCKED)
					{
						targetPos = fcs_->CalcPredictivePos(activeWp->GetBulletSpeed(), transform_.pos);
					}
					else
					{
						VECTOR camForward = GetCameraFrontVector();
						targetPos = VAdd(muzzlePos, VScale(camForward, activeWp->GetRange()));
					}

					activeWp->Fire(muzzlePos, targetPos, activeBullets_, false);

					if (requiredEN > 0) AudioManager::GetInstance()->PlaySE(SoundID::SE_BULLET_EN);
					else                AudioManager::GetInstance()->PlaySE(SoundID::SE_BULLET);

					en_ -= static_cast<float>(requiredEN);
				}
			}
		}
		else if (activeWeaponSlot_ == EquipSlot::R_BACK) // ─── 右肩武器 ───
		{
			if (isFireTrigger)
			{
				if (canFire)
				{
					VECTOR localMuzzlePos = VGet(40.0f, 180.0f, -20.0f);
					VECTOR muzzlePos = VAdd(transform_.pos, transform_.quaRot.PosAxis(localMuzzlePos));

					if (rMissileWp != nullptr && fcs_ != nullptr)
					{
						// 🚀 右肩が【ミサイル】の場合：マルチロック発射
						const std::vector<EnemyBase*>& targetEnemies = fcs_->GetLockTargets();
						if (!targetEnemies.empty())
						{
							rMissileWp->StartMultiLaunch(targetEnemies, muzzlePos);
							en_ -= static_cast<float>(requiredEN);
						}
					}
					else
					{
						// 💥 右肩が【キャノン（通常火器）】の場合：予測射撃
						VECTOR targetPos;
						if (fcs_ != nullptr && fcs_->GetLockState() == FCS::LOCK_STATE::LOCKED)
						{
							targetPos = fcs_->CalcPredictivePos(activeWp->GetBulletSpeed(), transform_.pos);
						}
						else
						{
							VECTOR camForward = GetCameraFrontVector();
							targetPos = VAdd(muzzlePos, VScale(camForward, activeWp->GetRange()));
						}

						activeWp->Fire(muzzlePos, targetPos, activeBullets_, false);

						if (requiredEN > 0) AudioManager::GetInstance()->PlaySE(SoundID::SE_CANNON_EN);
						else                AudioManager::GetInstance()->PlaySE(SoundID::SE_CANNON);

						en_ -= static_cast<float>(requiredEN);
					}
				}
			}
		}
		else if (activeWeaponSlot_ == EquipSlot::L_BACK) // ─── 左肩武器 ───
		{
			if (isFireTrigger)
			{
				if (canFire)
				{
					VECTOR localMuzzlePos = VGet(-40.0f, 180.0f, 20.0f);
					VECTOR muzzlePos = VAdd(transform_.pos, transform_.quaRot.PosAxis(localMuzzlePos));

					if (lMissileWp != nullptr && fcs_ != nullptr)
					{
						// 🚀 左肩が【ミサイル】の場合：マルチロック発射
						const std::vector<EnemyBase*>& targetEnemies = fcs_->GetLockTargets();
						if (!targetEnemies.empty())
						{
							lMissileWp->StartMultiLaunch(targetEnemies, muzzlePos);
							en_ -= static_cast<float>(requiredEN);
						}
					}
					else
					{
						// 💥 左肩が【キャノン（通常火器）】の場合：予測射撃 ＋ 構え硬直
						VECTOR targetPos;
						if (fcs_ != nullptr && fcs_->GetLockState() == FCS::LOCK_STATE::LOCKED)
						{
							targetPos = fcs_->CalcPredictivePos(activeWp->GetBulletSpeed(), transform_.pos);
						}
						else
						{
							VECTOR camForward = GetCameraFrontVector();
							targetPos = VAdd(muzzlePos, VScale(camForward, activeWp->GetRange()));
						}

						activeWp->Fire(muzzlePos, targetPos, activeBullets_, false);

						if (requiredEN > 0) AudioManager::GetInstance()->PlaySE(SoundID::SE_CANNON_EN);
						else                AudioManager::GetInstance()->PlaySE(SoundID::SE_CANNON);

						en_ -= static_cast<float>(requiredEN);

						// 【AC仕様】地上キャノン発射時の反動硬直
						if (isGrounded_)
						{
							ChangeState(STATE::STOP);
							boostMode_ = BOOST_MODE::BRAKE;
							stopTimer_ = 0.8f;
						}
					}
				}
			}
		}
	}


	// ==========================================================
	// 5. 弾丸の更新・削除処理（すり抜け防止・確定版）
	// ==========================================================
	int stageHandle = SceneManager::GetInstance().GetStageModelHandle();

	for (auto it = activeBullets_.begin(); it != activeBullets_.end(); )
	{
		if (*it == nullptr)
		{
			it = activeBullets_.erase(it);
			continue;
		}

		(*it)->Update(stageHandle);

		bool isHit = false;

		VECTOR bPrev = (*it)->GetPrevPos();
		VECTOR bCurr = (*it)->GetPos();
		float bRadius = (*it)->GetRadius();
		int bDamage = (*it)->GetDamage();

		if (enemyMng_ != nullptr)
		{
			const auto& enemies = enemyMng_->GetEemies();
			for (auto* enemy : enemies)
			{
				if (enemy != nullptr && enemy->CheckHitBullet(bPrev, bCurr, bRadius, bDamage))
				{
					isHit = true;
					break;
				}
			}
		}

		if ((*it)->IsDead() || isHit)
		{
			if ((*it)->GetExplosionRadius() > 0.0f)
			{
				VECTOR explodePos = (*it)->GetPos();
				float exRadius = (*it)->GetExplosionRadius();
				int exDamage = (*it)->GetExplosionDamage();

				if (enemyMng_ != nullptr)
				{
					const auto& enemies = enemyMng_->GetEemies();
					for (auto* enemy : enemies)
					{
						if (enemy != nullptr)
						{
							VECTOR enemyCenter = enemy->GetPos();
							enemyCenter.y += 80.0f;

							float dist = VSize(VSub(enemyCenter, explodePos));
							if (dist <= exRadius)
							{
								enemy->ApplyDamage(exDamage);
							}
						}
					}
				}

				ExplosionEffect eff;
				eff.pos = explodePos;
				eff.maxRadius = exRadius;
				eff.currentRadius = 5.0f;
				eff.life = 12;
				explosionEffects_.push_back(eff);
			}

			delete (*it);
			it = activeBullets_.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = explosionEffects_.begin(); it != explosionEffects_.end(); )
	{
		it->life--;
		it->currentRadius += (it->maxRadius - it->currentRadius) * 0.3f;

		if (it->life <= 0)
		{
			it = explosionEffects_.erase(it);
		}
		else
		{
			++it;
		}
	}
}


void Player::UpdateAntiMissile(void)
{
	// 迎撃タイマーが進んでいるならカウントを減らして終了（リロード中）
	if (antiMissileTimer_ > 0)
	{
		antiMissileTimer_--;
		return;
	}

	// SceneManager から現在ステージを飛んでいるすべての弾のリストを取得
	std::vector<Bullet*>& bulletList = SceneManager::GetInstance().GetBulletList();

	// ステージ内のすべての弾をループで走査
	for (auto* bullet : bulletList)
	{
		// 既に消滅している弾、自分が撃った弾、ミサイルではない通常の銃弾はスキップ
		if (bullet == nullptr || bullet->IsDead()) continue;
		if (!bullet->IsEnemyBullet()) continue;
		if (!bullet->IsMissile()) continue;

		// プレイヤーの胴体中心（やや高め）の座標を計算
		VECTOR playerCenter = transform_.pos;
		playerCenter.y += 160.0f;

		// ミサイルの現在位置を取得
		VECTOR missilePos = bullet->GetPos();

		// プレイヤーとミサイルの間の距離を計算
		VECTOR toMissile = VSub(missilePos, playerCenter);
		float dist = VSize(toMissile);

		// 迎撃射程（射程内）に入っているか判定
		if (dist <= antiMissileRange_)
		{
			// ★ ACらしい緊張感を出すための確率要素
			// 100% 確実に落としたい場合は、この if 文を削除して中身だけにしてください
			if ((rand() % 100) < 10)
			{
				// --- ⭕ 迎撃成功処理 ---

				// 1. ミサイルを強制死亡（撃墜）状態にする
				// これにより、GameScene側のループで安全にメモリ解放・削除されます
				bullet->SetDead();

				AntiMissileEffect eff;
				eff.start = playerCenter;
				eff.end = missilePos;
				eff.life = 5; // 5フレームだけ描画する
				antiMissileEffects_.push_back(eff);

				// 3. リロードタイマーをセット
				antiMissileTimer_ = antiMissileReloadFrame_;

				// 1フレームに何発も同時に落とさないように、1発落としたら今回のループを抜ける
				break;
			}
		}
	}
}

void Player::UpdateEnergy(float deltaTime)
{
	bool isConsuming = false;

	InputManager& input = InputManager::GetInstance();
	bool isDashKeyPress = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);

	// 1. ブーストダッシュ移動による消費（チャージング中でない場合のみ）
	if (boostMode_ == BOOST_MODE::DASH && isDashKeyPress && !isCharging_)
	{
		// 🔥 前フレームでブースターがOFFだった（＝今、点火した瞬間！）
		if (!isBoosterOn_)
		{
			AudioManager::GetInstance()->PlaySE(SoundID::SE_BOOST);
		}

		en_ -= EN_CONSUME_DASH * deltaTime;
		isConsuming = true;
	}

	// 2. ブースト上昇による消費（チャージング中でない場合のみ）
	if (isBoostAscent_ && !isCharging_)
	{
		// 🔥 ダッシュしていなくて、かつ前フレームでブースターがOFFだった場合
		if (!isConsuming && !isBoosterOn_)
		{
			AudioManager::GetInstance()->PlaySE(SoundID::SE_BOOST);
		}

		en_ -= EN_CONSUME_ASCENT * deltaTime;
		isConsuming = true;
	}

	// 3. ブースターを何も使っていない場合はENが自然回復する
	// （※エネルギー武器を撃ったフレームでも、ブースト中でなければAC同様に自然回復は走ってOKです）
	if (!isConsuming)
	{
		en_ += EN_RECOVER * deltaTime;

		if (en_ > MAX_EN)
		{
			en_ = MAX_EN;

			// 🔥 ENが最大まで全回復したらチャージング解除！
			if (isCharging_)
			{
				isCharging_ = false;
				// 【AC演出】チャージングが完了した時の「プシューン」というシステム音を鳴らすならここ！
				// AudioManager::GetInstance()->PlaySE(SoundID::SE_ENERGY_RECOVERED);
			}
		}
	}

	// ==========================================================
	// ⚡【新設】全要因対応型：EN一括クランプ ＆ チャージング判定
	// ==========================================================
	// ブースト消費、または武器消費によってENが底を突いた場合
	if (en_ <= 0.0f && !isCharging_)
	{
		en_ = 0.0f;
		isCharging_ = true; // 🔥 チャージング状態へ強制移行

		// ブースト中だった場合の各停止ペナルティ処理
		if (boostMode_ == BOOST_MODE::DASH)
		{
			if (isGrounded_) {
				ChangeState(STATE::STOP); // 地上なら急ブレーキ硬直へ
				boostMode_ = BOOST_MODE::BRAKE;
			}
			else {
				boostMode_ = BOOST_MODE::NORMAL; // 空中なら通常落下へ
				airDashTime_ = 0.0f;
			}
		}

		if (isBoostAscent_)
		{
			isBoostAscent_ = false; // 上昇強制停止
		}
	}

	// ※ ここで初めて今フレームの結果が保存されるため、
	// 次のフレームの処理の頭では「前フレームの状態」として扱えます。
	isBoosterOn_ = isConsuming;
}

void Player::UpdateStop(void)
{
	float deltaTime = scnMng_.GetDeltaTime();
	stopTimer_ -= deltaTime;

	if (stopTimer_ <= 0.0f) {
		ChangeState(STATE::PLAY);
	}
}

void Player::UpdateLandingStiff(void)
{
	float deltaTime = scnMng_.GetDeltaTime();
	landingStiffTimer_ -= deltaTime;

	// 硬直中も XZ軸の残存エネルギーがあれば摩擦で強制停止させる
	movePow_.x *= (1.0f - 0.2f * deltaTime * 60.0f);
	movePow_.z *= (1.0f - 0.2f * deltaTime * 60.0f);

	// 時間が来たら通常のプレイ状態に復帰
	if (landingStiffTimer_ <= 0.0f) {
		ChangeState(STATE::PLAY);
	}
}

void Player::CollisionReserve(void)
{
	// アニメーションごとの線分調整
	if (animationController_->GetPlayType() == static_cast<int>(ANIM_TYPE::JUMP))
	{
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::LINE)) != 0)
		{
			ColliderLine* colLine = dynamic_cast<ColliderLine*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::LINE)));
			colLine->SetLocalPosStart(COL_LINE_JUMP_START_LOCAL_POS);
			colLine->SetLocalPosEnd(COL_LINE_JUMP_END_LOCAL_POS);
		}

		// ジャンプ中はカプセルを上に上げる
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::CAPSULE)) != 0)
		{
			ColliderCapsule* colCapsule = dynamic_cast<ColliderCapsule*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::CAPSULE)));
			colCapsule->SetLocalPosTop(COL_CAPSULE_TOP_JUMP_LOCAL_POS);
			colCapsule->SetLocalPosDown(COL_CAPSULE_DOWN_JUMP_LOCAL_POS);
			colCapsule->SetRadius(COL_CAPSULE_RADIUS);
		}
	}
	else
	{
		// 通常時の線分に戻す
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::LINE)) != 0)
		{
			ColliderLine* colLine = dynamic_cast<ColliderLine*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::LINE)));
			colLine->SetLocalPosStart(COL_LINE_START_LOCAL_POS);
			colLine->SetLocalPosEnd(COL_LINE_END_LOCAL_POS);
		}

		// 通常時のカプセルに戻す
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::CAPSULE)) != 0)
		{
			ColliderCapsule* colCapsule = dynamic_cast<ColliderCapsule*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::CAPSULE)));
			colCapsule->SetLocalPosTop(COL_CAPSULE_TOP_LOCAL_POS);
			colCapsule->SetLocalPosDown(COL_CAPSULE_DOWN_LOCAL_POS);
			colCapsule->SetRadius(COL_CAPSULE_RADIUS);
		}
	}
}

void Player::Draw2D(void)
{
	if ((state_ == STATE::PLAY || state_ == STATE::STOP || state_ == STATE::LANDING_STIFF) && fcs_ != nullptr)
	{
		fcs_->Draw();

		int screenWidth, screenHeight;
		GetDrawScreenSize(&screenWidth, &screenHeight);

		// =========================================================
		// 【左下】ENゲージの描画 (既存のまま)
		// =========================================================
		int enX = 80;
		int enY = screenHeight - 120;
		int enWidth = 200;
		int enHeight = 12;
		float enRatio = en_ / MAX_EN;
		DrawBox(enX, enY, enX + enWidth, enY + enHeight, GetColor(40, 40, 40), TRUE);

		unsigned int enColor = (enRatio < 0.2f) ? GetColor(255, 64, 64) : GetColor(0, 255, 128);
		if (isCharging_) enColor = GetColor(255, 0, 0);

		DrawBox(enX, enY, enX + static_cast<int>(enWidth * enRatio), enY + enHeight, enColor, TRUE);
		DrawBox(enX, enY, enX + enWidth, enY + enHeight, GetColor(200, 200, 200), FALSE);

		if (isCharging_)
		{
			if ((GetNowCount() / 200) % 2 == 0) DrawString(enX, enY - 18, "CHARGING...", GetColor(255, 0, 0));
		}
		else
		{
			DrawString(enX, enY - 18, "EN GAUGE", GetColor(255, 255, 255));
		}


		// =========================================================
		// 【右下】★使用中の武器・残弾数UIのみを描画する（ゲージなし版）
		// =========================================================
		WeaponBase* activeWp = GetActiveWeapon();

		if (activeWp != nullptr)
		{
			int wpX = screenWidth - 240; // 右端からの位置を微調整
			int wpY = screenHeight - 100; // ゲージが消えた分、少し下げて配置

			int currentAmmo = activeWp->GetCurrentAmmo();

			// ① 武器名の描画
			DrawString(wpX, wpY - 22, activeWp->GetName().c_str(), GetColor(255, 255, 255));

			// ② 残弾数のデジタル数値表示 (アクティブなスロットに応じて文字色を識別)
			unsigned int textColor = (activeWeaponSlot_ == EquipSlot::R_BACK) ? GetColor(255, 128, 0) : GetColor(0, 255, 255);

			// ⚡ AC風演出：弾数が 0 の時はグレーアウト、10発未満の時は赤文字警告にする
			if (currentAmmo == 0)
			{
				textColor = GetColor(128, 128, 128);
			}
			else if (currentAmmo < 10)
			{
				textColor = GetColor(255, 64, 64);
			}

			// 「AMMO: 085」のように、%03d で3桁固定表示にするとミリタリー感が出て引き締まります
			DrawFormatString(wpX, wpY, textColor, "AMMO: %03d", currentAmmo);
		}
	}
}

bool Player::CheckHitBullet(const VECTOR& bulletPrevPos, const VECTOR& bulletPos, float bulletRadius, int damage)
{
	if (hp_ <= 0) return false;

	// CharactorBase等で定義されているコライダの取得
	int capsuleKey = static_cast<int>(CharactorBase::COLLIDER_TYPE::CAPSULE);
	const auto& ownColliders = GetOwnColliders();

	if (ownColliders.count(capsuleKey) > 0)
	{
		auto* baseCollider = ownColliders.at(capsuleKey);
		if (baseCollider != nullptr && baseCollider->IsValid()) // 有効フラグもチェック
		{
			// ⚡ ご提示いただいた ColliderCapsule クラスへ安全にキャスト
			auto* capsuleCollider = dynamic_cast<ColliderCapsule*>(baseCollider);
			if (capsuleCollider != nullptr)
			{
				// ⚡ 実際のゲッター名（GetPosTop, GetPosDown, GetRadius）を適用
				VECTOR charTop    = capsuleCollider->GetPosTop();   // カプセル上部球体のワールド座標
				VECTOR charDown   = capsuleCollider->GetPosDown();  // カプセル下部球体のワールド座標
				float charRadius  = capsuleCollider->GetRadius();   // キャラクターの判定半径

				// ⚡ カプセル vs カプセル (HitCheckCapsuleCapsule) で超高速弾のすり抜けを完全に防ぐ
				// 引数: カプセル1の線分両端・半径、カプセル2の線分両端・半径
				if (HitCheck_Capsule_Capsule(bulletPrevPos, bulletPos, bulletRadius, 
										   charTop, charDown, charRadius) == 1)
				{
					// 被弾処理 (ダメージ適用など)
					ApplyDamage(damage); 
					return true; // 衝突した
				}
			}
		}
	}
	return false; // 衝突しなかった
}

WeaponBase* Player::GetActiveWeapon(void) const
{
	switch (activeWeaponSlot_)
	{
	case EquipSlot::R_ARM:  return rightWeapon_;
	case EquipSlot::R_BACK: return rightBackWeapon_;
	case EquipSlot::L_BACK: return leftBackWeapon_; // ⚡ここが正しく leftBackWeapon_ を返しているか
	}
	return nullptr;
}
